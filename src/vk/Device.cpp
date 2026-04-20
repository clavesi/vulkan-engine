#include "Device.h"
#include "Instance.h"
#include "../core/Engine.h"

namespace {
    // Store graphics card extensions we require
    const std::vector<const char *> requiredDeviceExtensions = {
        vk::KHRSwapchainExtensionName
    };
} // namespace

Device::Device(const Instance &instance, const vk::raii::SurfaceKHR& surface) {
    pickPhysicalDevice(instance.get(), surface);
    createLogicalDevice();

    // Load device-level function pointers for faster dispatch
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
}

bool Device::isDeviceSuitable(
    const vk::raii::PhysicalDevice &candidate,
    const vk::raii::SurfaceKHR &surf
) const {
    // Vulkan 1.3
    const bool supportsVulkan1_3 = candidate.getProperties().apiVersion >= vk::ApiVersion13;

    // Check if any of the queue families support graphics operations
    const auto queueFamilies = candidate.getQueueFamilyProperties();
    bool supportsGraphicsAndPresent = false;
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        const bool graphics = static_cast<bool>(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics);
        const bool present = candidate.getSurfaceSupportKHR(i, *surf);
        if (graphics && present) {
            supportsGraphicsAndPresent = true;
            break;
        }
    }

    // Check if all required physicalDevice extensions are available
    const auto availableExts = candidate.enumerateDeviceExtensionProperties();
    const bool supportsAllRequiredExtensions = std::ranges::all_of(
        requiredDeviceExtensions,
        [&availableExts](const char *required) {
            return std::ranges::any_of(
                availableExts,
                [required](const auto &available) {
                    return std::strcmp(available.extensionName, required) == 0;
                });
        });

    // Check if the physicalDevice supports the required features.
    const auto features = candidate.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    const bool supportsRequiredFeatures =
            features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
            features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
            features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

    // Return true if the physicalDevice meets all the criteria
    return supportsVulkan1_3
           && supportsGraphicsAndPresent
           && supportsAllRequiredExtensions
           && supportsRequiredFeatures;
}


void Device::pickPhysicalDevice(
    const vk::raii::Instance &inst,
    const vk::raii::SurfaceKHR &surf
) {
    std::vector<vk::raii::PhysicalDevice> physicalDevices = inst.enumeratePhysicalDevices();
    auto const iter = std::ranges::find_if(
        physicalDevices,
        [this, &surf](const auto &pd) {
            return isDeviceSuitable(pd, surf);
        });
    if (iter == physicalDevices.end()) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    physicalDevice = *iter;

    // Find the queue family we'll use. Same check as in isDeviceSuitable, but now we record the index.
    const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        const bool graphics = static_cast<bool>(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics);
        const bool present = physicalDevice.getSurfaceSupportKHR(i, *surf);
        if (graphics && present) {
            queueIndex = i;
            break;
        }
    }
    if (queueIndex == ~0u) {
        throw std::runtime_error("no queue family supporting graphics and present");
    }
}

void Device::createLogicalDevice() {
    // Query for 1.1 and 1.3 features
    // Structure chain automatically connects these structures together by setting up pNext pointers between them
    // Default-construct the chain and then set the feature fields explicitly. Initializer-list / designated
    // initializers don't match StructureChain's constructors (clangd/clang error), so assign members via .get<T>().
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > featureChain;
    // Leave PhysicalDeviceFeatures2 empty
    // Enable shader draw parameters required by SPIR-V DrawParameters capability
    featureChain.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters = VK_TRUE;
    // Enable dynamic rendering from Vulkan 1.3
    featureChain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = VK_TRUE;
    // Enable synchronization2 for vkCmdPipelineBarrier2 and related APIs
    featureChain.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 = VK_TRUE;
    // Enable extended dynamic state from the extension
    featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState = VK_TRUE;

    // Create a device
    float queuePriority = 1.0f; // influence scheduling of command buffer (0.0-1.0)
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = queueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data()
    };

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    queue = vk::raii::Queue(device, queueIndex, 0);
}
