#include "Buffer.h"
#include "Device.h"

#include <stdexcept>

Buffer::Buffer(
    const Device &device, const vk::DeviceSize size,
    const vk::BufferUsageFlags usage, const vk::MemoryPropertyFlags properties
) : device(device), bufferSize(size) {
    const vk::BufferCreateInfo bufferInfo{
        .size = size,
        .usage = usage,
        // Exclusive access from the graphics queue
        .sharingMode = vk::SharingMode::eExclusive
    };

    // Create buffer
    buffer = vk::raii::Buffer{device.logical(), bufferInfo};

    const auto memRequirements = buffer.getMemoryRequirements();
    // Allocate memory
    const vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    // Handle to memory
    memory = vk::raii::DeviceMemory{device.logical(), allocInfo};

    // Associate this memory with the buffer
    buffer.bindMemory(*memory, 0);
}

void Buffer::uploadData(const void *src, const vk::DeviceSize size) const {
    if (size > bufferSize) {
        throw std::runtime_error("Upload exceeds buffer size!");
    }
    // Map buffer memory into CPU accessible memory
    void *dst = memory.mapMemory(0, size);
    std::memcpy(dst, src, size);
    memory.unmapMemory();
    // Driver may not immediately copy data into buffer memory.
    // This is dealt with by either flushing,
    // or what we do, using MemoryPropertyFlagBits::eHostCoherent (Renderer & findMemoryType())
}

void Buffer::uploadViaStaging(const void *src, const vk::DeviceSize size) const {
    if (size > bufferSize) {
        throw std::runtime_error("Upload exceeds buffer size!");
    }

    // Temporary CPU-visible buffer we can map and write into
    const Buffer staging(
        device,
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    staging.uploadData(src, size);

    // GPU-side copy from staging into this buffer's device-local memory
    device.copyBuffer(staging.handle(), buffer, size);

    // staging is destroyed here when the scope ends
}

void *Buffer::mapPersistent() {
    // Only map once; subsequent calls return the existing pointer
    if (!persistentMap) {
        persistentMap = memory.mapMemory(0, bufferSize);
    }
    return persistentMap;
}
