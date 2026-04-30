#include "Device.h"
#include "Instance.h"
#include "../core/Engine.h"

namespace {
    // Store graphics card extensions we require
    const std::vector<const char *> requiredDeviceExtensions = {
        vk::KHRSwapchainExtensionName
    };
} // namespace

Device::Device(const Instance &instance, const vk::raii::SurfaceKHR &surface) {
    pickPhysicalDevice(instance.get(), surface);
    createLogicalDevice();
    createTransientCommandPool();

    // Load device-level function pointers for faster dispatch
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
}

// Graphics card can offer different types of memory to allocate from.
// Combining requirements of the buffer and our app's requirements, we find the right type of memory to use.
uint32_t Device::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
    // Get device's types of memories
    const auto memPropertiess = physicalDevice.getMemoryProperties();
    // Find suitable memory
    for (uint32_t i = 0; i < memPropertiess.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memPropertiess.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void Device::copyBuffer(const vk::raii::Buffer &src, const vk::raii::Buffer &dst, vk::DeviceSize size) const {
    auto cmd = beginSingleTimeCommands();

    cmd.copyBuffer(src, dst, vk::BufferCopy{0, 0, size});

    endSingleTimeCommands(cmd);
}

vk::raii::CommandBuffer Device::beginSingleTimeCommands() const {
    // Allocate a single-use command buffer from the transient pool
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = transientPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    auto cmdBuffers = vk::raii::CommandBuffers(device, allocInfo);
    auto cmd = std::move(cmdBuffers.front());

    // Tell the driver this buffer is recorded once and submitted once
    cmd.begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    });

    return cmd;
}

void Device::endSingleTimeCommands(const vk::raii::CommandBuffer &cmd) const {
    cmd.end();

    // Submit and block until the copy finishes (no fences needed for a one-shot)
    queue.submit(
        vk::SubmitInfo{
            .commandBufferCount = 1,
            .pCommandBuffers = &*cmd
        }, nullptr
    );
    queue.waitIdle();
}

vk::Format Device::findSupportedFormat(
    const std::vector<vk::Format> &candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features
) const {
    for (const auto format: candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

vk::Format Device::findDepthFormat() const {
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
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
            features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
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
    // Sampler anisotropy lives on the base PhysicalDeviceFeatures2
    featureChain.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = vk::True;
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

// eTransient hints to the driver that command buffers from this pool are short-lived
void Device::createTransientCommandPool() {
    vk::CommandPoolCreateInfo info{
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = queueIndex
    };
    transientPool = vk::raii::CommandPool(device, info);
}
