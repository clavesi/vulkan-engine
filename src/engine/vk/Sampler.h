#pragma once

#include <vulkan/vulkan_raii.hpp>

class Device;

class Sampler {
public:
    // Default-configured sampler suitable for typical 2D textures: linear filtering, repeat addressing, full anisotropy.
    explicit Sampler(const Device &device, float maxLod = 0.0f);

    Sampler(const Sampler &) = delete;
    Sampler &operator=(const Sampler &) = delete;

    [[nodiscard]] const vk::raii::Sampler &handle() const { return sampler; }

private:
    vk::raii::Sampler sampler = nullptr;
};
