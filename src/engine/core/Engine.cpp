#include "Engine.h"

#include <chrono>

#include "Vertex.h"
#include "core/MeshGenerator.h"

#include <fstream>

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
      unlitPipeline(
          device,
          makeMainPipelineSpec(config.unlitShaderPath, swapChain.format(), swapChain.depthFormat(), swapChain.samples())
      ),
      renderer(device, swapChain, pipeline, config, scene) {
    initScene();
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
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!window.shouldClose()) {
        // Delta time
        const auto currentTime = std::chrono::high_resolution_clock::now();
        const float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        window.pollEvents();

        const bool resized = window.wasResized();
        if (resized) window.resetResizedFlag();

        // Feed per-frame input to camera
        camera.onMouseDrag(window.getMouseDelta());
        camera.onScroll(window.getScrollDelta());

        scene.update(deltaTime);

        const auto [width, height] = window.getFramebufferSize();
        const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

        renderer.drawFrame(
            camera.getViewMatrix(),
            camera.getProjectionMatrix(aspectRatio),
            glm::vec3{0.0f, 0.0f, 0.0f}, // light (sun)
            camera.getPosition(),
            resized
        );

        window.resetFrameInput();
    }

    device.waitIdle();
}

void Engine::initScene() {
    // Load mesh
    auto [vertices, indices] = MeshGenerator::sphere(1.0f, 32, 32);
    meshes.emplace_back(device, vertices, indices);
    const Mesh &sphere = meshes.back();

    // Sun - stationary, unlit
    scene.addObject(sphere, unlitPipeline, Transform{.scale = {4.0f, 4.0f, 4.0f}});

    // Planet 1 — orbits at radius 3, one full revolution per 5 seconds
    scene.addObject(
        sphere, pipeline,
        Transform{.position = {10.0f, 0.0f, 0.0f}, .scale = {0.5f, 0.5f, 0.5f}},
        OrbitalBody{.radius = 10.0f, .speed = glm::two_pi<float>() / 5.0f}
    );

    // Planet 2 — orbits at radius 5, one full revolution per 10 seconds
    scene.addObject(
        sphere, pipeline,
        Transform{.position = {30.0f, 0.0f, 0.0f}, .scale = {1.0f, 1.0f, 1.0f}},
        OrbitalBody{.radius = 30.0f, .speed = glm::two_pi<float>() / 10.0f}
    );
}
