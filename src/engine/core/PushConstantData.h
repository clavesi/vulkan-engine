#pragma once

#include <glm/glm.hpp>

struct PushConstantData {
    glm::mat4 model;
    uint32_t  objectId = 0;
};
