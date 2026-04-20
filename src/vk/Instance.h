#pragma once

#include <vulkan/vulkan_raii.hpp>

class Instance {
public:
    Instance();

    Instance(const Instance &) = delete;
    Instance &operator=(const Instance &) = delete;

    // Access to the underlying RAII handle. Other classes (Device, Window.createSurface())
    // need this to constructor their own Vulkan objects
    [[nodiscard]] const vk::raii::Instance &get() const { return instance; }
    [[nodiscard]] const vk::raii::Context &context() const { return ctx; }

private:
    void createInstance();
    void setupDebugMessenger();

    static std::vector<const char *> getRequiredInstanceExtensions();
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData
    );

    // Declaration order matters: context -> instance -> debugMessenger.
    // Data member to hold the handle to the instance and raii context
    vk::raii::Context ctx;
    vk::raii::Instance instance = nullptr;
    // Set up callback function for debug messaging
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
};
