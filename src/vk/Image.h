#pragma once

#include <vulkan/vulkan_raii.hpp>

class Device;

class Image {
public:
    Image(const Device &device,
          uint32_t width, uint32_t height,
          vk::Format format,
          vk::ImageTiling tiling,
          vk::ImageUsageFlags usage,
          vk::MemoryPropertyFlags properties);

    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;
    Image(Image &&) noexcept = default;
    Image &operator=(Image &&) = delete;

    // Records and submits a one-shot pipeline barrier to change the image's layout.
    // Currently handles two transitions: undefined → transfer dst,and transfer dst → shader read.
    void transitionLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

    // Copies pixel data from a host-visible buffer into this image.
    // The image must already be in eTransferDstOptimal layout.
    void copyFromBuffer(const vk::raii::Buffer &src,
                        uint32_t width, uint32_t height);

    [[nodiscard]] const vk::raii::Image &handle() const { return image; }
    [[nodiscard]] vk::Format format() const { return imageFormat; }
    [[nodiscard]] uint32_t width() const { return imageWidth; }
    [[nodiscard]] uint32_t height() const { return imageHeight; }

private:
    const Device &device;

    vk::raii::Image image = nullptr;
    vk::raii::DeviceMemory memory = nullptr;
    vk::Format imageFormat;
    uint32_t imageWidth;
    uint32_t imageHeight;
};
