#pragma once

#include <vulkan/vulkan_raii.hpp>

class Device;

class Buffer {
public:
    Buffer(const Device &device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    Buffer(Buffer &&) noexcept = default;
    Buffer &operator=(Buffer &&) = delete; // // can't reseat the Device& reference

    // Copies bytes from src into the buffer. Requires buffer's memory to be host-visible
    void uploadData(const void *src, vk::DeviceSize size) const;
    // Upload to a device-local buffer by going through a temporary host-visible
    // staging buffer. Use when this buffer's memory is not CPU-mappable.
    void uploadViaStaging(const void *src, vk::DeviceSize size) const;

    [[nodiscard]] const vk::raii::Buffer &handle() const { return buffer; }
    [[nodiscard]] vk::DeviceSize size() const { return bufferSize; }

    // Map memory once and keep it mapped for the buffer's lifetime.
    // Useful for buffers written every frame (e.g. uniform buffers) since
    // mapping/unmapping repeatedly is more expensive than the memcpy itself.
    void *mapPersistent();

private:
    const Device &device;

    vk::raii::Buffer buffer = nullptr;
    vk::raii::DeviceMemory memory = nullptr;
    vk::DeviceSize bufferSize;

    void *persistentMap = nullptr;
};
