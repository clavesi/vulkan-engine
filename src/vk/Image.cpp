#include "Image.h"

#include "Device.h"

Image::Image(const Device &device, uint32_t width, uint32_t height,
             vk::Format format, vk::ImageTiling tiling,
             vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
    : device(device), imageFormat(format), imageWidth(width), imageHeight(height) {
    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };
    image = vk::raii::Image(device.logical(), imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    memory = vk::raii::DeviceMemory(device.logical(), allocInfo);
    image.bindMemory(*memory, 0);
}

void Image::transitionLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
    auto cmd = device.beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier{
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    };

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (
        oldLayout == vk::ImageLayout::eUndefined &&
        newLayout == vk::ImageLayout::eTransferDstOptimal
    ) {
        // Transfer writes don't have to wait on anything
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (
        oldLayout == vk::ImageLayout::eTransferDstOptimal &&
        newLayout == vk::ImageLayout::eShaderReadOnlyOptimal
    ) {
        // Fragment shader reads must wait for transfer writes to finish
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("unsupported layout transition");
    }

    cmd.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);

    device.endSingleTimeCommands(cmd);
}

void Image::copyFromBuffer(const vk::raii::Buffer &src, uint32_t width, uint32_t height) {
    auto cmd = device.beginSingleTimeCommands();

    vk::BufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0, // tightly packed
        .bufferImageHeight = 0,
        .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1}
    };

    cmd.copyBufferToImage(src, image, vk::ImageLayout::eTransferDstOptimal, region);

    device.endSingleTimeCommands(cmd);
}
