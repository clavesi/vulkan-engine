#pragma once

#include <glm/glm.hpp>

struct PushConstantData {
    glm::mat4 model;
    uint32_t  objectId = 0;
    // 12 bytes implicit padding to next 16-byte boundary — total 80 bytes
};