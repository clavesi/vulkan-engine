#pragma once

#include <cstdint>
#include <string>

struct EngineConfig {
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;
    std::string windowTitle = "Vulkan";
    std::string shaderPath = "shaders/shader.spv";
    std::string texturePath = "textures/viking_room.png";
    std::string modelPath = "models/viking_room.obj";

    // Camera initial state
    float cameraDistance = 3.0f;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.3f; // slight downward tilt to see the model
    float cameraFov = 45.0f;
    float cameraNear = 0.1f;
    float cameraFar = 1000.0f;
};
