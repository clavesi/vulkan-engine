#pragma once

#include <cstdint>
#include <string>

struct EngineConfig {
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;
    std::string windowTitle = "Vulkan";
    std::string shaderPath = "shaders/shader.spv";
};
