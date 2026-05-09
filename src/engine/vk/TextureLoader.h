#pragma once

#include "Texture.h"
#include <string>

class Device;

namespace vk_util {
    void loadTexture(const Device &device, const std::string &path, Texture &out);
}
