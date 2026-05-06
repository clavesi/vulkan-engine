#include "Engine.h"

#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <iostream>

#include "core/MeshGenerator.h"
#include "Vertex.h"

namespace {
    PipelineSpec makeMainPipelineSpec(const std::string &shaderPath, const vk::Format colorFormat,
                                      const vk::Format depthFormat, const vk::SampleCountFlagBits samples) {
        const auto attrs = Vertex::getAttributeDescriptions();

        // Vertex shader reads MVP matrices from the UBO
        vk::DescriptorSetLayoutBinding uboBinding{
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        };

        // Fragment shader samples colors from the texture
        vk::DescriptorSetLayoutBinding samplerBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = colorFormat,
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = {attrs.begin(), attrs.end()},
            .descriptorBindings = {uboBinding, samplerBinding},
            .depthFormat = depthFormat,
            .samples = samples,
            .pushConstantSize = sizeof(glm::mat4),
        };
    }
} // namespace

Engine::Engine(EngineConfig cfg)
    : config(std::move(cfg)),
      camera(config.cameraDistance, config.cameraYaw, config.cameraPitch,
             config.cameraFov, config.cameraNear, config.cameraFar),
      window(config.windowWidth, config.windowHeight, config.windowTitle),
      instance(),
      surface(window.createSurface(instance.get())),
      device(instance, surface),
      swapChain(device, window, surface),
      pipeline(
          device,
          makeMainPipelineSpec(config.shaderPath, swapChain.format(), swapChain.depthFormat(), swapChain.samples())
      ),
      renderer(device, swapChain, pipeline, config) {
    buildScene();
}

Engine::~Engine() {
    // Make sure all GPU work is done before any member destructors run.
    // *device checks that device was actually created (it may be null if
    // construction failed partway).
    device.waitIdle();
}

void Engine::run() {
    mainLoop();
}

void Engine::mainLoop() {
    while (!window.shouldClose()) {
        window.pollEvents();

        const bool resized = window.wasResized();
        if (resized) window.resetResizedFlag();

        // Feed per-frame input to camera
        camera.onMouseDrag(window.getMouseDelta());
        camera.onScroll(window.getScrollDelta());

        const auto [width, height] = window.getFramebufferSize();
        const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

        renderer.drawFrame(
            camera.getViewMatrix(),
            camera.getProjectionMatrix(aspectRatio),
            glm::vec3{5.0f, 5.0f, 5.0f}, // light (sun)
            camera.getPosition(),
            resized
        );

        window.resetFrameInput();
    }
    device.waitIdle(); // wait for device to finish operations before destroying resources
}

void Engine::buildScene() {
    // // Load the model from config
    // auto [vertices,indices] = io::loadObj(config.modelPath);
    // meshes.emplace_back(device, vertices, indices);
    // // Add it to the scene at the origin with no transform
    // renderer.addObject(meshes.back());
    // renderer.addObject(meshes.back(), Transform{.position = {2.0f, 0.0f, 0.0f}});

    auto [vertices, indices] = MeshGenerator::sphere(1.0f, 32, 32);
    meshes.emplace_back(device, vertices, indices);
    renderer.addObject(meshes.back());
    // Second sphere offset to the side
    renderer.addObject(meshes.back(), Transform{.position = {3.0f, 0.0f, 0.0f}});
}
