#include "Engine.h"
#include "PipelineSpecs.h"
#include "Vertex.h"
#include "vk/TextureLoader.h"
#include "core/MeshGenerator.h"
#include "io/GltfLoader.h"

#include <imgui.h>

#include <chrono>
#include <fstream>
#include <iostream>

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
          PipelineSpecs::makeMain(config.shaderPath, swapChain.format(), swapChain.depthFormat(), swapChain.samples())
      ),
      unlitPipeline(
          device,
          PipelineSpecs::makeUnlit(config.unlitShaderPath, swapChain.format(), swapChain.depthFormat(),
                                   swapChain.samples())
      ),
      pickingPipeline(
          device,
          PipelineSpecs::makePicking(config.pickingShaderPath, swapChain.depthFormat(), swapChain.samples())
      ),
      outlinePipeline(
          device,
          PipelineSpecs::makeOutline(config.outlineShaderPath, swapChain.format(), swapChain.depthFormat(),
                                     swapChain.samples())
      ),
      orbitPipeline(
          device,
          PipelineSpecs::makeOrbit(config.orbitShaderPath, swapChain.format(), swapChain.depthFormat(),
                                   swapChain.samples())
      ),
      skyboxPipeline(
          device,
          PipelineSpecs::makeSkybox(config.skyboxShaderPath, swapChain.format(), swapChain.depthFormat(),
                                    swapChain.samples())
      ),
      earthPipeline(
          device,
          PipelineSpecs::makeEarth(config.earthShaderPath, swapChain.format(), swapChain.depthFormat(),
                                   swapChain.samples())
      ),
      renderer(
          device, swapChain, pipeline, pickingPipeline, outlinePipeline, orbitPipeline, skyboxPipeline,
          earthPipeline, config, scene, input
      ),
      imguiRenderer(device, swapChain, instance.get(), window.glfwHandle()) {
    input.init(window.glfwHandle());
    imguiRenderer.initGlfw(window.glfwHandle());
    initScene();
    renderer.setSkybox(*skyboxMesh, skyboxTexture);
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
        // Log-space speed slider
        float logSpeed = std::log10(simSpeed);
        ImGui::Text("Speed: %.2fx", simSpeed);
        ImGui::SetNextItemWidth(100.0f);
        if (ImGui::SliderFloat("##speed", &logSpeed, -2.0f, 1.0f, "")) {
            simSpeed = std::pow(10.0f, logSpeed);
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
        ImGui::SetNextWindowSize(ImVec2(150, 0), ImGuiCond_Always);
        ImGui::Begin("##planets", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoBringToFrontOnFocus
        );
        const auto &objects = scene.getObjects();
        for (size_t i = 0; i < objects.size(); ++i) {
            const auto &obj = objects[i];
            // Only top-level objects (no parent) in main loop
            if (obj.parentIndex) continue;
            if (obj.name.empty()) continue;

            const bool isFollowing = camera.isFollowing() && camera.getFollowObjectId() == i;
            if (isFollowing) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
            if (ImGui::Button(obj.name.c_str(), ImVec2(120, 0))) {
                camera.setFollowTarget(static_cast<uint32_t>(i));
                camera.setDesiredDistance(obj.transform.scale.x * 8.0f);
            }
            if (isFollowing) ImGui::PopStyleColor();

            // Moons — find children of this planet and indent buttons
            for (size_t j = 0; j < objects.size(); ++j) {
                const auto &moon = objects[j];
                if (!moon.parentIndex || *moon.parentIndex != i) continue;

                ImGui::Indent(12.0f);
                const bool moonFollowing = camera.isFollowing() && camera.getFollowObjectId() == j;
                ImGui::PushStyleColor(
                    ImGuiCol_Text,
                    moonFollowing
                        ? ImVec4(0.4f, 0.7f, 1.0f, 1.0f) // highlight when following
                        : ImVec4(0.7f, 0.7f, 0.7f, 1.0f) // dimmed otherwise
                );
                if (ImGui::Selectable(moon.name.c_str(), moonFollowing, 0, ImVec2(104, 0))) {
                    camera.setFollowTarget(static_cast<uint32_t>(j));
                    camera.setDesiredDistance(moon.transform.scale.x * 8.0f);
                }
                ImGui::PopStyleColor();
                ImGui::Unindent(12.0f);
            }
        }
        ImGui::End();

        // Planet info panel — shown when following a planet
        if (camera.isFollowing()) {
            const uint32_t id = camera.getFollowObjectId();
            if (id < objects.size() && objects[id].bodyDef) {
                const auto *def = objects[id].bodyDef;

                ImGui::SetNextWindowPos(ImVec2(static_cast<float>(config.windowWidth) - 210.0f, 10.0f),
                                        ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(200, 0), ImGuiCond_Always);
                ImGui::Begin("##planetinfo", nullptr,
                             ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_NoBringToFrontOnFocus
                );

                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", def->name.c_str());
                ImGui::Separator();
                ImGui::Text("Radius:  %.0f km", def->radiusKm);
                ImGui::Text("Day:     %.2f Earth days", def->rotationDays);
                ImGui::Text("Year:    %.2f Earth days", def->periodDays);

                ImGui::End();
            }
        }

        scene.update(paused ? 0.0f : deltaTime * simSpeed);

        // Update follow target position each frame
        if (camera.isFollowing()) {
            const uint32_t id = camera.getFollowObjectId();
            if (id < scene.getObjects().size()) {
                camera.setTargetImmediate(scene.getObjects()[id].transform.position);
            }
        }

        camera.onScroll(input.getScrollDelta());
        camera.update(deltaTime);

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
    // Skybox
    auto [skyVerts, skyIndices] = MeshGenerator::cube();
    skyboxMesh.emplace(device, skyVerts, skyIndices);
    vk_util::loadTexture(device, config.skyboxTexturePath, skyboxTexture);

    // Load mesh
    auto [sphereVertices, sphereIndices] = MeshGenerator::sphere(1.0f, 64, 64);
    meshes.emplace_back(device, sphereVertices, sphereIndices);
    const Mesh &sphere = meshes.back();

    auto [asteroidVertices, asteroidIndices] = io::loadGltfMesh(config.asteroidModelPath);
    meshes.emplace_back(device, asteroidVertices, asteroidIndices);
    const Mesh &asteroidMesh = meshes.back();

    solarSystem.init(
        scene, sphere, asteroidMesh,
        pipeline, unlitPipeline, earthPipeline,
        textures, device, orbitMeshes
    );
}
