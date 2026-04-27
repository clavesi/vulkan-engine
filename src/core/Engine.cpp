#include "Engine.h"

#include <stdexcept>
#include <cassert>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include "Vertex.h"

#include <GLFW/glfw3.h>

namespace {
    PipelineSpec makeMainPipelineSpec(const std::string &shaderPath, vk::Format colorFormat) {
        const auto attrs = Vertex::getAttributeDescriptions();

        // Every binding needs to be described through a VkDescriptorSetLayoutBinding struct.
        // The vertex shader reads MVP matrices from a uniform buffer at binding 0
        vk::DescriptorSetLayoutBinding uboBinding{
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex,
            nullptr
        };

        return PipelineSpec{
            .shaderPath = shaderPath,
            .colorFormat = colorFormat,
            .bindingDescription = Vertex::getBindingDescription(),
            .attributeDescriptions = {attrs.begin(), attrs.end()},
            .descriptorBindings = {uboBinding}
        };
    }
} // namespace

Engine::Engine(EngineConfig cfg)
    : config(std::move(cfg)),
      window(config.windowWidth, config.windowHeight, config.windowTitle),
      instance(),
      surface(window.createSurface(instance.get())),
      device(instance, surface),
      swapChain(device, window, surface),
      pipeline(device, makeMainPipelineSpec(config.shaderPath, swapChain.format())),
      renderer(device, swapChain, pipeline) {
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
        if (resized) {
            window.resetResizedFlag();
        }

        renderer.drawFrame();
    }
    device.waitIdle(); // wait for device to finish operations before destroying resources
}
