#pragma once

#include "Config.h"
#include "Camera.h"
#include "Window.h"
#include "Input.h"
#include "vk/Instance.h"
#include "vk/Device.h"
#include "vk/SwapChain.h"
#include "vk/Pipeline.h"
#include "vk/Renderer.h"
#include "core/Mesh.h"
#include "core/Scene.h"
#include "ui/ImGuiRenderer.h"
#include "app/SolarSystem.h"

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <list>

class Engine {
public:
    Engine(EngineConfig cfg = {});
    ~Engine();

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    void run();

private:
    void mainLoop();

    void initScene();

    // Declaration order = construction order.
    //   window    -> needed for surface and for getting framebuffer size
    //   instance  -> Vulkan entry point
    //   surface   -> created from window + instance; needed by device
    //   device    -> needs instance and surface
    EngineConfig config;
    Camera camera;
    Input input;
    Window window;
    Instance instance;
    // Use the Window System Integration (WSI) to create a surface to present rendered images to
    // Needs to be created right after the instance creation since it can influence the physical device selection.
    vk::raii::SurfaceKHR surface;
    Device device;
    SwapChain swapChain;
    Pipeline pipeline; // lit
    Pipeline unlitPipeline; // unlit
    Pipeline pickingPipeline; // picking for detecting object mouse over
    Pipeline outlinePipeline; // outline for object mouse over
    Pipeline orbitPipeline; // draw planets' orbit
    Pipeline skyboxPipeline; // skybox
    std::vector<Mesh> meshes; // owns mesh data
    std::list<Texture> textures; // owns texture data
    Scene scene;
    Renderer renderer; // holds raw pointers into meshes
    ImGuiRenderer imguiRenderer;

    std::list<Mesh> orbitMeshes;
    std::optional<Mesh> skyboxMesh;
    Texture skyboxTexture;

    bool paused = false;

    app::SolarSystem solarSystem;
};
