#include "Engine.h"

#include <fstream>

#define GLFW_INCLUDE_VULKAN
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
            vk::ShaderStageFlagBits::eVertex,
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
            .samples = samples
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
      pipeline(
          device,
          makeMainPipelineSpec(config.shaderPath, swapChain.format(), swapChain.depthFormat(), swapChain.samples())
      ),
      renderer(device, swapChain, pipeline, config) {
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

        if (window.wasResized()) {
            window.resetResizedFlag();
        }

        renderer.drawFrame();
    }
    device.waitIdle(); // wait for device to finish operations before destroying resources
}
