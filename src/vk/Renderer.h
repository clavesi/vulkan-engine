#pragma once

#include "Buffer.h"
#include "../core/UniformBufferObject.h"

#include <vulkan/vulkan_raii.hpp>

#include <vector>

class Device;
class SwapChain;
class Pipeline;

class Renderer {
public:
    Renderer(const Device &device, SwapChain &swapChain, const Pipeline &pipeline);

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    // Renders one frame. Handles swapchain recreation on out-of-date.
    // externalResize: set true when the OS reports a resize that Vulkan
    // may not have noticed yet (e.g. via GLFW framebuffer callback).
    void drawFrame(bool externalResize = false);

private:
    // Allows the rendering of one frame to not interfere with the recording of the next.
    // Essentially, CPU and GPU work separately and with a value of 2. If the CPU finishes early, it waits for the GPU to finish rendering.
    // With more, the CPU might get ahead of the GPU, adding frames of latency.
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void transitionImageLayout(
        uint32_t imageIndex,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask
    );
    void recordCommandBuffer(uint32_t imageIndex);

    void updateUniformBuffer(uint32_t frameIdx);

    const Device &device;
    SwapChain &swapChain; // non-const because drawFrame may trigger recreate()
    const Pipeline &pipeline;

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

    Buffer vertexBuffer;
    // uint32_t vertexCount = 0; // currently unused
    Buffer indexBuffer;
    uint32_t indexCount = 0;
};
