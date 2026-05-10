#include "Engine.h"
#include "Vertex.h"
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
        const std::vector<vk::VertexInputAttributeDescription> unlitAttrs = {
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
        const std::vector<vk::VertexInputAttributeDescription> pickingAttrs = {
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
        const std::vector<vk::VertexInputAttributeDescription> outlineAttrs = {
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
          makeOutlinePipelineSpec(config.outlineShaderPath, swapChain.format(), swapChain.depthFormat(),
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

        Window::pollEvents();
        imguiRenderer.beginFrame();

        const bool resized = window.wasResized();
        if (resized) window.resetResizedFlag();

        camera.onScroll(input.getScrollDelta());
        camera.update(deltaTime);

        const bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse;
        if (!imguiWantsMouse) {
            if (input.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
                camera.onMouseDrag(input.getMouseDelta());
            }
            // Double click to change follow target - double-click only makes sense in orbit mode
            if (!camera.isFreeCam() && input.wasDoubleClicked(GLFW_MOUSE_BUTTON_LEFT)) {
                const uint32_t hovered = renderer.getHoveredObjectId();
                if (hovered != UINT32_MAX) {
                    camera.setFollowTarget(hovered);
                    camera.setDesiredDistance(scene.getObjects()[hovered].transform.scale.x * 8.0f);
                }
            }
        }

        const bool imguiWantsKeyboard = ImGui::GetIO().WantCaptureKeyboard;
        if (!imguiWantsKeyboard) {
            if (input.wasKeyPressed(GLFW_KEY_ESCAPE)) {
                if (camera.isFreeCam()) {
                    camera.toggleFreeCam(); // back to orbit
                } else {
                    camera.resetTarget(); // existing reset behavior
                }
            }
            if (input.wasKeyPressed(GLFW_KEY_SPACE)) {
                paused = !paused;
            }
            if (input.wasKeyPressed(GLFW_KEY_F)) {
                camera.toggleFreeCam();
            }

            // WASD movement in free cam
            if (camera.isFreeCam()) {
                glm::vec3 move{0.0f};
                if (input.isKeyDown(GLFW_KEY_D)) move.x += 1.0f;
                if (input.isKeyDown(GLFW_KEY_A)) move.x -= 1.0f;
                if (input.isKeyDown(GLFW_KEY_W)) move.y += 1.0f;
                if (input.isKeyDown(GLFW_KEY_S)) move.y -= 1.0f;
                if (input.isKeyDown(GLFW_KEY_E)) move.z += 1.0f;
                if (input.isKeyDown(GLFW_KEY_Q)) move.z -= 1.0f;
                camera.onFreeCamMove(move, deltaTime);
            }
        }

        // Build controls UI
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(120, 0), ImGuiCond_Always);
        ImGui::Begin("##controls", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoBringToFrontOnFocus
        );
        if (ImGui::Button(paused ? "  Resume" : "  Pause", ImVec2(100, 34))) {
            paused = !paused;
        }
        if (camera.isFreeCam()) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "  Free Cam");
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  F to exit");
        } else {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "  Orbit Cam");
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  F to fly");
        }
        const float controlsBottom = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;
        ImGui::End();

        // Planet list panel — anchored below controls window
        ImGui::SetNextWindowPos(ImVec2(10, controlsBottom + 8), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(140, 0), ImGuiCond_Always);
        ImGui::Begin("##planets", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoBringToFrontOnFocus
        );
        const auto &objects = scene.getObjects();
        for (size_t i = 0; i < objects.size(); ++i) {
            const auto &obj = objects[i];
            if (obj.name.empty()) continue;
            const bool isFollowing = camera.isFollowing() && camera.getFollowObjectId() == i;
            if (isFollowing) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
            if (ImGui::Button(obj.name.c_str(), ImVec2(120, 0))) {
                camera.setFollowTarget(static_cast<uint32_t>(i));
                camera.setDesiredDistance(obj.transform.scale.x * 8.0f);
            }
            if (isFollowing) ImGui::PopStyleColor();
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
    auto [vertices, indices] = MeshGenerator::sphere(1.0f, 64, 64);
    meshes.emplace_back(device, vertices, indices);
    const Mesh &sphere = meshes.back();

    solarSystem.init(scene, sphere, pipeline, unlitPipeline, textures, device);
}
