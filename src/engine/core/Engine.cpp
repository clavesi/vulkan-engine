#include "Engine.h"
#include "Vertex.h"
#include "vk/TextureLoader.h"
#include "core/MeshGenerator.h"

#include <imgui.h>

#include <chrono>
#include <fstream>
#include <iostream>

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
            .pushConstantSize = sizeof(glm::mat4) + sizeof(uint32_t),
        };
    }

    PipelineSpec makeUnlitPipelineSpec(const std::string &shaderPath,
                                       const vk::Format colorFormat,
                                       const vk::Format depthFormat,
                                       const vk::SampleCountFlagBits samples) {
        // Same as lit spec but with only 3 attributes — no normal
        const auto attrs = Vertex::getAttributeDescriptions();

        vk::DescriptorSetLayoutBinding uboBinding{
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        };
        vk::DescriptorSetLayoutBinding samplerBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };

        // Only locations 0, 2, 3 — skip normal at location 1
        std::vector<vk::VertexInputAttributeDescription> unlitAttrs = {
            attrs[0], // location 0: pos
            attrs[2], // location 2: color
            attrs[3], // location 3: texcoord
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = colorFormat,
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = unlitAttrs,
            .descriptorBindings = {uboBinding, samplerBinding},
            .depthFormat = depthFormat,
            .samples = samples,
            .pushConstantSize = sizeof(glm::mat4) + sizeof(uint32_t),
        };
    }

    PipelineSpec makePickingPipelineSpec(
        const std::string &shaderPath,
        const vk::Format depthFormat,
        const vk::SampleCountFlagBits samples
    ) {
        std::vector<vk::VertexInputAttributeDescription> pickingAttrs = {
            Vertex::getAttributeDescriptions()[0], // location 0: pos only
        };

        vk::DescriptorSetLayoutBinding uboBinding{
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        };
        vk::DescriptorSetLayoutBinding samplerBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = vk::Format::eR32Uint, // picking image format
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = pickingAttrs,
            .descriptorBindings = {uboBinding, samplerBinding},
            .depthFormat = depthFormat,
            .samples = vk::SampleCountFlagBits::e1,
            .pushConstantSize = sizeof(glm::mat4) + sizeof(uint32_t), // no MSAA for picking
            .colorAttachmentFormat = vk::Format::eR32Uint,
        };
    }

    PipelineSpec makeOutlinePipelineSpec(
        const std::string &shaderPath,
        const vk::Format colorFormat,
        const vk::Format depthFormat,
        const vk::SampleCountFlagBits samples
    ) {
        const auto attrs = Vertex::getAttributeDescriptions();
        std::vector<vk::VertexInputAttributeDescription> outlineAttrs = {
            attrs[0], // location 0: pos
            attrs[1], // location 1: normal
        };

        vk::DescriptorSetLayoutBinding uboBinding{
            0, vk::DescriptorType::eUniformBuffer, 1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr
        };
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
            .attributeDescriptions = outlineAttrs,
            .descriptorBindings = {uboBinding, samplerBinding},
            .depthFormat = depthFormat,
            .samples = samples,
            .pushConstantSize = sizeof(glm::mat4) + sizeof(uint32_t),
            .cullMode = vk::CullModeFlagBits::eFront,
            .depthTestEnable = true,
            .depthWriteEnable = false,
            .depthCompareOp = vk::CompareOp::eLess,
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
          makeUnlitPipelineSpec(config.unlitShaderPath, swapChain.format(), swapChain.depthFormat(),
                                swapChain.samples())
      ),
      pickingPipeline(
          device,
          makePickingPipelineSpec(config.pickingShaderPath, swapChain.depthFormat(), swapChain.samples())
      ),
      outlinePipeline(
          device,
          makeOutlinePipelineSpec("shaders/shader_outline.spv", swapChain.format(), swapChain.depthFormat(),
                                  swapChain.samples())
      ),
      renderer(device, swapChain, pipeline, pickingPipeline, outlinePipeline, config, scene, input),
      imguiRenderer(device, swapChain, instance.get(), window.glfwHandle()) {
    input.init(window.glfwHandle());
    imguiRenderer.initGlfw(window.glfwHandle());
    initScene();
    renderer.onSceneReady();
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
        imguiRenderer.beginFrame();

        const bool resized = window.wasResized();
        if (resized) window.resetResizedFlag();

        camera.onScroll(input.getScrollDelta());
        camera.update(deltaTime);

        const bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse;
        const bool imguiWantsKeyboard = ImGui::GetIO().WantCaptureKeyboard;
        if (!imguiWantsMouse) {
            if (input.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
                camera.onMouseDrag(input.getMouseDelta());
            }
            // Double click to change follow target
            if (input.wasDoubleClicked(GLFW_MOUSE_BUTTON_LEFT)) {
                const uint32_t hovered = renderer.getHoveredObjectId();
                if (hovered != UINT32_MAX) {
                    camera.setFollowTarget(hovered);
                    const auto &obj = scene.getObjects()[hovered];
                    camera.setDesiredDistance(obj.transform.scale.x * 5.0f);
                }
            }
        }

        if (!imguiWantsKeyboard) {
            if (input.wasKeyPressed(GLFW_KEY_ESCAPE)) {
                camera.resetTarget();
            }
            if (input.wasKeyPressed(GLFW_KEY_SPACE)) {
                paused = !paused;
            }
        }

        // Build pause UI
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(120, 50), ImGuiCond_Always);
        ImGui::Begin("##controls", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoBringToFrontOnFocus
        );
        if (ImGui::Button(paused ? "  Resume" : "  Pause", ImVec2(100, 34))) {
            paused = !paused;
        }
        ImGui::End();

        // Update follow target position each frame
        if (camera.isFollowing()) {
            const uint32_t id = camera.getFollowObjectId();
            if (id < scene.getObjects().size()) {
                camera.setTargetImmediate(scene.getObjects()[id].transform.position);
            }
        }

        scene.update(paused ? 0.0f : deltaTime);

        const auto [width, height] = window.getFramebufferSize();
        const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

        const glm::vec2 contentScale = window.getContentScale();
        renderer.drawFrame(
            camera.getViewMatrix(),
            camera.getProjectionMatrix(aspectRatio),
            glm::vec3{0.0f, 0.0f, 0.0f},
            camera.getPosition(),
            contentScale,
            imguiRenderer,
            resized
        );

        input.reset();
    }

    device.waitIdle();
}

void Engine::initScene() {
    // Load mesh
    auto [vertices, indices] = MeshGenerator::sphere(1.0f, 32, 32);
    meshes.emplace_back(device, vertices, indices);
    const Mesh &sphere = meshes.back();

    // Load shared texture for now — each object gets its own pointer,
    // ready for per-object textures once we have more assets
    textures.emplace_back();
    vk_util::loadTexture(device, config.texturePath, textures.back());
    const Texture &tex = textures.back();

    // Sun - stationary, unlit
    scene.addObject(sphere, unlitPipeline, tex, Transform{.scale = {4.0f, 4.0f, 4.0f}});

    // Planet 1 — orbits at radius 3, one full revolution per 5 seconds
    scene.addObject(
        sphere, pipeline, tex,
        Transform{.position = {10.0f, 0.0f, 0.0f}, .scale = {0.5f, 0.5f, 0.5f}},
        OrbitalBody{.radius = 10.0f, .speed = glm::two_pi<float>() / 5.0f}
    );

    // Planet 2 — orbits at radius 5, one full revolution per 10 seconds
    scene.addObject(
        sphere, pipeline, tex,
        Transform{.position = {30.0f, 0.0f, 0.0f}, .scale = {1.0f, 1.0f, 1.0f}},
        OrbitalBody{.radius = 30.0f, .speed = glm::two_pi<float>() / 10.0f}
    );
}
