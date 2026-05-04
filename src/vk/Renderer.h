#pragma once

#include "core/Config.h"
#include "core/Mesh.h"
#include "Buffer.h"
#include "Image.h"
#include "Sampler.h"

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

#include <vector>
#include<optional>

class Device;
class SwapChain;
class Pipeline;

class Renderer {
public:
    Renderer(const Device &device, SwapChain &swapChain, const Pipeline &pipeline, const EngineConfig &config);

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    // Renders one frame. Handles swapchain recreation on out-of-date.
    // externalResize: set true when the OS reports a resize that Vulkan
    // may not have noticed yet (e.g. via GLFW framebuffer callback).
    void drawFrame(glm::mat4 view, glm::mat4 proj, bool externalResize = false);

private:
    // Allows the rendering of one frame to not interfere with the recording of the next.
    // Essentially, CPU and GPU work separately and with a value of 2. If the CPU finishes early, it waits for the GPU to finish rendering.
    // With more, the CPU might get ahead of the GPU, adding frames of latency.
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void transitionImageLayout(
        vk::Image image,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask
    ) const;
    void recordCommandBuffer(uint32_t imageIndex) const;

    void updateUniformBuffer(uint32_t frameIdx, glm::mat4 view, glm::mat4 proj) const;

    void createDescriptorPool();
    void createDescriptorSets();

    void createTextureImage();

    const Device &device;
    SwapChain &swapChain; // non-const because drawFrame may trigger recreate()
    const Pipeline &pipeline;
    const EngineConfig &config;

    // Manage memory used to store buffers and command buffers allocated from them
    vk::raii::CommandPool commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;
    uint32_t frameIndex = 0;

    // One uniform buffer per frame in flight so the CPU can write
    // the next frame's data without disturbing what the GPU is currently reading
    std::vector<Buffer> uniformBuffers;
    std::vector<void *> uniformBuffersMapped;

    // Pool that descriptor sets are allocated from
    vk::raii::DescriptorPool descriptorPool = nullptr;
    // One descriptor set per frame in flight, each pointing at that frame's uniform buffer
    std::vector<vk::raii::DescriptorSet> descriptorSets;

    std::optional<Mesh> mesh;

    std::optional<Image> textureImage;
    vk::raii::ImageView textureImageView = nullptr;
    std::optional<Sampler> textureSampler;
};
