#pragma once

#include <vulkan/vulkan_raii.hpp>

class Instance; // forward declare

class Device {
public:
    Device(const Instance &instance, const vk::raii::SurfaceKHR &surface);

    Device(const Device &) = delete;

    Device &operator=(const Device &) = delete;

    [[nodiscard]] const vk::raii::PhysicalDevice &physical() const { return physicalDevice; }
    [[nodiscard]] const vk::raii::Device &logical() const { return device; }
    [[nodiscard]] const vk::raii::Queue &graphicsQueue() const { return queue; }
    [[nodiscard]] uint32_t queueFamilyIndex() const { return queueIndex; }

    void waitIdle() const { device.waitIdle(); }

    [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
    // Submit a one-time copy command between two buffers
    void copyBuffer(const vk::raii::Buffer &src, const vk::raii::Buffer &dst, vk::DeviceSize size) const;

    // Begin recording a one-shot command buffer for transient operations (uploads, layout transitions, etc.).
    // Returned buffer is in the recording state — issue commands, then call endSingleTimeCommands.
    [[nodiscard]] vk::raii::CommandBuffer beginSingleTimeCommands() const;
    // End, submit, and wait. Caller must not use the buffer after this.
    void endSingleTimeCommands(const vk::raii::CommandBuffer &cmd) const;

private:
    [[nodiscard]] bool isDeviceSuitable(
        const vk::raii::PhysicalDevice &candidate,
        const vk::raii::SurfaceKHR &surf
    ) const;

    void pickPhysicalDevice(
        const vk::raii::Instance &inst,
        const vk::raii::SurfaceKHR &surf
    );
    void createLogicalDevice();

    void createTransientCommandPool();

    // Store graphics card
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    // Store logical device handle and features
    vk::raii::Device device = nullptr;
    vk::PhysicalDeviceFeatures deviceFeatures; // currently unused. used in later sections
    // Store graphics queue family handle
    vk::raii::Queue queue = nullptr;
    uint32_t queueIndex = ~0u;

    // Pool for short-lived command buffers like one-time staging transfers
    vk::raii::CommandPool transientPool = nullptr;
};
