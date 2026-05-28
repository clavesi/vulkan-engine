#pragma once

#include "Texture.h"
#include <string>

class Device;

namespace vk_util {
    void loadTexture(const Device &device, const std::string &path, Texture &out);

    void loadTextureFromMemory(const Device &device, const uint8_t *data, size_t size, Texture &out);
}
