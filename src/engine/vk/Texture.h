#pragma once

#include "Image.h"
#include "Sampler.h"

#include <vulkan/vulkan_raii.hpp>

#include <optional>

struct Texture {
    std::optional<Image> image;
    vk::raii::ImageView view = nullptr;
    std::optional<Sampler> sampler;

    Texture() = default;
    Texture(Texture &&) = default;
    Texture &operator=(Texture &&) = default;
};
