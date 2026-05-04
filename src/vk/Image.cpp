#include "Image.h"

#include "Device.h"

Image::Image(const Device &device, const uint32_t width, const uint32_t height,
             const uint32_t mipLevels, vk::SampleCountFlagBits samples, const vk::Format format,
             const vk::ImageTiling tiling, const vk::ImageUsageFlags usage, const vk::MemoryPropertyFlags properties)
    : device(device), mipLevels(mipLevels), samples(samples),
      imageFormat(format), imageWidth(width), imageHeight(height) {
    const vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = samples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };
    image = vk::raii::Image(device.logical(), imageInfo);

    const vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    const vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    memory = vk::raii::DeviceMemory(device.logical(), allocInfo);
    image.bindMemory(*memory, 0);
}

void Image::transitionLayout(const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout) const {
    const auto cmd = device.beginSingleTimeCommands();

    // Determine aspect from the new layout (depth-related layouts use eDepth)
    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
    if (newLayout == vk::ImageLayout::eDepthAttachmentOptimal ||
        newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        aspect = vk::ImageAspectFlagBits::eDepth;
    }

    vk::ImageMemoryBarrier barrier{
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {aspect, 0, mipLevels, 0, 1}
    };

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (
        oldLayout == vk::ImageLayout::eUndefined
        && newLayout == vk::ImageLayout::eTransferDstOptimal
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
    } else if (
        oldLayout == vk::ImageLayout::eUndefined
        && newLayout == vk::ImageLayout::eDepthAttachmentOptimal
    ) {
        // Depth attachment writes happen in the early fragment test stage
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        throw std::invalid_argument("unsupported layout transition");
    }

    cmd.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);

    device.endSingleTimeCommands(cmd);
}

void Image::copyFromBuffer(const vk::raii::Buffer &src, const uint32_t width, const uint32_t height) const {
    const auto cmd = device.beginSingleTimeCommands();

    const vk::BufferImageCopy region{
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

vk::raii::ImageView Image::createView(const vk::ImageAspectFlags aspect) const {
    // Aspect is eColor for textures, eDepth for depth buffers
    const vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = imageFormat,
        .subresourceRange = {aspect, 0, mipLevels, 0, 1}
    };
    return vk::raii::ImageView(device.logical(), viewInfo);
}

void Image::generateMipmaps(vk::Format format, int32_t texWidth, int32_t texHeight) const {
    // Linear blit isn't guaranteed to be supported for every format.
    // For real engines you'd either fall back to a software resize (stb_image_resize)
    // or pre-bake mips into the texture file.
    // Here, we just require the support
    const auto formatProps = device.formatProperties(format);
    if (!(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not support linear blitting");
    }

    const auto cmd = device.beginSingleTimeCommands();

    // Barrier template; per-level fields (baseMipLevel, layouts, access masks) are filled in below
    vk::ImageMemoryBarrier barrier{
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = *image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0, // overwritten per iteration
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        // Transition level i-1 from eTransferDstOptimal to eTransferSrcOptimal so we can blit from it.
        // Waits on the previous blit's write (or the initial buffer copy for i=1)
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            {}, {}, {},
            barrier
        );

        // Blit from level i-1 (full size) into level i (half-size; clamp to 1 to handle non-square images)
        const vk::ImageBlit blit{
            .srcSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .srcOffsets = std::array{
                vk::Offset3D{0, 0, 0},
                vk::Offset3D{mipWidth, mipHeight, 1}
            },
            .dstSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .dstOffsets = std::array{
                vk::Offset3D{0, 0, 0},
                vk::Offset3D{
                    mipWidth > 1 ? mipWidth / 2 : 1,
                    mipHeight > 1 ? mipHeight / 2 : 1,
                    1
                }
            }
        };

        // Source and destination are the same image — different mip levels
        cmd.blitImage(
            *image, vk::ImageLayout::eTransferSrcOptimal,
            *image, vk::ImageLayout::eTransferDstOptimal,
            blit,
            vk::Filter::eLinear
        );

        // Transition level i-1 to eShaderReadOnlyOptimal — it's done being read from
        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {}, {}, {}, barrier
        );

        // Halve dims for the next iteration; clamp at 1 so non-square images don't go to 0
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // Final mip level was never blitted from, so it's still in eTransferDstOptimal — transition it now
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        {}, {}, {}, barrier
    );

    device.endSingleTimeCommands(cmd);
}
