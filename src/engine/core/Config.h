#pragma once

#include <cstdint>
#include <string>

struct EngineConfig {
    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 720;
    std::string windowTitle = "Vulkan";
    std::string shaderPath = "shaders/shader.spv";
    std::string unlitShaderPath = "shaders/shader_unlit.spv";
    std::string pickingShaderPath = "shaders/picking.spv";
    std::string outlineShaderPath = "shaders/shader_outline.spv";
    std::string orbitShaderPath = "shaders/shader_orbit.spv";
    std::string skyboxShaderPath = "shaders/shader_skybox.spv";
    std::string texturePath = "textures/viking_room.png";
    std::string skyboxTexturePath = "textures/solar/hiptyc_2020_16k_gal.jpg";
    std::string modelPath = "models/viking_room.obj";

    // Camera initial state
    float cameraDistance = 50.0f;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.3f; // slight downward tilt to see the model
    float cameraFov = 45.0f;
    float cameraNear = 0.001f;
    float cameraFar = 20000.0f;
};
