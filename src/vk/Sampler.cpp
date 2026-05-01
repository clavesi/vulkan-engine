#include "Sampler.h"
#include "Device.h"

Sampler::Sampler(const Device &device) {
    const auto props = device.properties();

    const vk::SamplerCreateInfo info{
        // Linear filtering smooths magnified/minified textures (vs eNearest blocky)
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        // Repeat the texture when UVs go outside [0, 1]
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        // Anisotropic filtering reduces blur on textures viewed at sharp angles
        .anisotropyEnable = vk::True,
        .maxAnisotropy = props.limits.maxSamplerAnisotropy,
        // No depth comparison (used for shadow maps, not regular sampling)
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        // Mipmap range; we don't use mipmaps yet so both are 0
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        // Use [0, 1] UV coordinates rather than raw pixel coordinates
        .unnormalizedCoordinates = vk::False
    };

    sampler = vk::raii::Sampler(device.logical(), info);
}
