#include "Instance.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Defines storage for vk::detail::defaultDispatchLoaderDynamic.
// Must appear in exactly one translation unit.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace {
    // What validation layers
    const std::vector<char const *> validationLayers{
        "VK_LAYER_KHRONOS_validation"
    };

    // and whether to enable them
#ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
#else
    constexpr bool enableValidationLayers = true;
#endif
} // namespace

Instance::Instance() {
    // Create instance, the connection between the Engine and the Vulkan lib
    createInstance();
    setupDebugMessenger();
}

void Instance::createInstance() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init(); // loads vkGetInstanceProcAddr

    vk::ApplicationInfo appInfo{
        .pApplicationName = "Vulkan",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    // Get the required layers
    std::vector<const char *> requiredLayers;
    if (enableValidationLayers) {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }
    // Check if the required layers are supported by the Vulkan implementation
    auto layerProperties = ctx.enumerateInstanceLayerProperties();
    const auto unsupportedLayerIt = std::ranges::find_if(
        requiredLayers,
        [&layerProperties](const char *requiredLayer) {
            return std::ranges::none_of(
                layerProperties,
                [requiredLayer](const auto &layerProperty) {
                    return strcmp(layerProperty.layerName, requiredLayer) == 0;
                });
        });
    if (unsupportedLayerIt != requiredLayers.end()) {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }

    // Get the required extensions
    auto requiredExtensions = getRequiredInstanceExtensions();
    // Check if the required extensions are supported by the Vulkan implementation
    auto extensionProperties = ctx.enumerateInstanceExtensionProperties();
    const auto unsupportedPropertyIt = std::ranges::find_if(
        requiredExtensions,
        [&extensionProperties](const char *requiredExtension) {
            return std::ranges::none_of(
                extensionProperties,
                [requiredExtension](const auto &extensionProperty) {
                    return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
                });
        });
    if (unsupportedPropertyIt != requiredExtensions.end()) {
        throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
    }

    const vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data()
    };

    // Create instance
    instance = vk::raii::Instance(ctx, createInfo);

    // Load instance-level functions (vkGetPhysicalDeviceProperties, etc.)
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
}

void Instance::setupDebugMessenger() {
    if constexpr (!enableValidationLayers) return;

    // Fill in struct with details about the messenger and its callback
    constexpr vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    constexpr vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    constexpr vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        // Pointer to callback function
        .pfnUserCallback = &debugCallback
        // Optionally, pass a pointer to the pUserData field which will be passed to the callback function via that parameter
        // Use this, for example, to pass a pointer to the mai class.
    };

    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

// Returns list of the required instance extensions
std::vector<const char *> Instance::getRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    // extensions specified by GLFW are always required when using it for windowing
    const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {
        // debug messenger extension
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
}

/*
 * INPUT:
 * severity: flag that specifies severity of message (eVerbose, eInfo, eWarning, eError)
 *          setup so you can check if a message is equal or worse than another
 *          i.e. severity >= ...::eWarning {} will show both warning and error messages
 * type: flag for whether message relates to Vulkan specs or not (eGeneral, eValidation, ePerformance)
 * pCallbackData: struct with message details like the debug message and Vulkan object handles related to it
 * pUserData: pointer specified during setup of the callback and allows passing in your own data
 *
 * OUTPUT:
 * Boolean that indicates if the Vulkan call that triggered the validation layer message should be aborted.
 */
VKAPI_ATTR vk::Bool32 VKAPI_CALL Instance::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    const vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData
) {
    std::cerr << "validation layer: type " << to_string(type)
            << " msg: " << pCallbackData->pMessage << std::endl;

    return vk::False;
}
