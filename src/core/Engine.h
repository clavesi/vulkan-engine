#pragma once

#include "Config.h"
#include "Camera.h"
#include "Window.h"
#include "vk/Instance.h"
#include "vk/Device.h"
#include "vk/SwapChain.h"
#include "vk/Pipeline.h"
#include "vk/Renderer.h"

#include <vulkan/vulkan_raii.hpp>

class Engine {
public:
    Engine(EngineConfig cfg = {});
    ~Engine();

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    void run();

private:
    void mainLoop();

    // Declaration order = construction order.
    //   window    -> needed for surface and for getting framebuffer size
    //   instance  -> Vulkan entry point
    //   surface   -> created from window + instance; needed by device
    //   device    -> needs instance and surface
    EngineConfig config;
    Camera camera;
    Window window;
    Instance instance;
    // Use the Window System Integration (WSI) to create a surface to present rendered images to
    // Needs to be created right after the instance creation since it can influence the physical device selection.
    vk::raii::SurfaceKHR surface;
    Device device;
    SwapChain swapChain;
    Pipeline pipeline;
    Renderer renderer;
};
