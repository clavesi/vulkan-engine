#include "TextureLoader.h"
#include "Buffer.h"
#include "Device.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace vk_util {
    void loadTexture(const Device &device, const std::string &path, Texture &out) {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(
            path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha
        );
        if (!pixels) throw std::runtime_error("failed to load texture: " + path);

        const vk::DeviceSize imageSize = texWidth * texHeight * 4;
        const uint32_t mipLevels =
                static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        const Buffer staging(
            device, imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        staging.uploadData(pixels, imageSize);
        stbi_image_free(pixels);

        out.image.emplace(
            device,
            static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight),
            mipLevels, vk::SampleCountFlagBits::e1,
            vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );
        out.image->transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        out.image->copyFromBuffer(staging.handle(), static_cast<uint32_t>(texWidth),
                                  static_cast<uint32_t>(texHeight));
        out.image->generateMipmaps(vk::Format::eR8G8B8A8Srgb, texWidth, texHeight);

        out.view = out.image->createView();
        out.sampler.emplace(device, vk::LodClampNone);
    }
}
