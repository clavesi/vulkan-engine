#pragma once

// GLM defaults to OpenGL's [-1, 1] depth range; Vulkan uses [0, 1]
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
// Force GLM to use std140-compatible alignment for vec/mat types. Saves us from having to write alignas(16) on every member.
// BE AWARE: this guard does NOT fix nested structs — those still need explicit alignas. Always be explicit when in doubt.
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

// Per-frame transformation data passed to the vertex shader.
// Layout must match the UniformBuffer struct in shader.slang.
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
