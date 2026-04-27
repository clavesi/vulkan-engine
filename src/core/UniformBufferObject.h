#pragma once

#include <glm/glm.hpp>

// Per-frame transformation data passed to the vertex shader.
// Layout must match the UniformBuffer struct in shader.slang.
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
