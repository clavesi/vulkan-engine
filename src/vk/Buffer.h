#pragma once


#include <vulkan/vulkan_raii.hpp>

class Device;

class Buffer {
public:
    Buffer(const Device &device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;

    // Copies bytes from src into the buffer. Requires buffer's memory to be host-visible
    void uploadData(const void *src, vk::DeviceSize size);

    [[nodiscard]] const vk::raii::Buffer &handle() const { return buffer; }
    [[nodiscard]] vk::DeviceSize size() const { return bufferSize; }

private:
    const Device &device;

    vk::raii::Buffer buffer = nullptr;
    vk::raii::DeviceMemory memory = nullptr;
    vk::DeviceSize bufferSize;
};
