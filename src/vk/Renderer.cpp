#include "Renderer.h"
#include "Device.h"
#include "SwapChain.h"
#include "Pipeline.h"
#include "core/Vertex.h"
#include "core/UniformBufferObject.h"

#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cassert>
#include <stdexcept>
#include <chrono>

namespace {
    // top left = = red, top right = green, bottom right = blue, bottom left = white
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    // Index buffers allows you to reorder the vertex buffer, allowing reuse of data for multiple vertex.
    // For example, a rectangle, split into two triangles, would have six vertices. But they share the two on the diagonal.
    // With index buffer, we only need to define them once in the vertex buffer and then use it twice in the index buffer.
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };
}

Renderer::Renderer(const Device &device, SwapChain &swapChain, const Pipeline &pipeline)
    : device(device),
      swapChain(swapChain),
      pipeline(pipeline),
      // Vertex buffer now lives in fast device-local memory.
      // eTransferDst is required because we'll copy into it from a staging buffer.
      vertexBuffer(
          device,
          sizeof(vertices[0]) * vertices.size(),
          vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
          vk::MemoryPropertyFlagBits::eDeviceLocal),
      // vertexCount(static_cast<uint32_t>(vertices.size())),
      indexBuffer(
          device,
          sizeof(indices[0]) * indices.size(),
          vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
          vk::MemoryPropertyFlagBits::eDeviceLocal),
      indexCount(static_cast<uint32_t>(indices.size())) {
    // Device-local memory isn't CPU-mappable, so go through a staging buffer
    vertexBuffer.uploadViaStaging(
        vertices.data(),
        sizeof(vertices[0]) * vertices.size()
    );
    indexBuffer.uploadViaStaging(
        indices.data(),
        sizeof(indices[0]) * indices.size()
    );

    createTextureImage();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();

    // Reserve so emplace_back doesn't try to move existing Buffers around
    uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.reserve(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Host-visible because we write to it every frame from the CPU
        uniformBuffers.emplace_back(
            device,
            sizeof(UniformBufferObject),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
        // Persistent mapping: get the pointer once, reuse it forever
        uniformBuffersMapped.push_back(uniformBuffers.back().mapPersistent());
    }

    createDescriptorPool();
    createDescriptorSets();
}

void Renderer::drawFrame(const bool externalResize) {
    // Note: inFlightFences, presentCompleteSemaphores, and commandBuffers are indexed by frameIndex,
    //       while renderFinishedSemaphores is indexed by imageIndex
    const auto fenceResult = device.logical().waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }

    // grab image from framebuffer after previous frame has finished
    // timeout essentially never (uint64 max)
    auto [acquireResult, imageIndex] = swapChain.handle().acquireNextImage(
        UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

    // Due to VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS being defined, eErrorOutOfDateKHR can be checked as a result
    // here and does not need to be caught by an exception.
    if (acquireResult == vk::Result::eErrorOutOfDateKHR) {
        swapChain.recreate();
        return;
    }
    // On other success codes than eSuccess and eSuboptimalKHR we just throw an exception.
    // On any error code, acquireNextImage already threw an exception.
    if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Only reset the fence if we are submitting work
    device.logical().resetFences(*inFlightFences[frameIndex]);

    // Refresh the MVP matrices for this frame before recording the draw
    updateUniformBuffer(frameIndex);

    commandBuffers[frameIndex].reset();
    recordCommandBuffer(imageIndex);

    // Submit command buffer
    vk::PipelineStageFlags waitDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
        .pWaitDstStageMask = &waitDstStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex]
    };
    device.graphicsQueue().submit(submitInfo, *inFlightFences[frameIndex]);

    // Presentation
    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChain.handle(),
        .pImageIndices = &imageIndex
    };

    vk::Result presentResult;
    try {
        presentResult = device.graphicsQueue().presentKHR(presentInfoKHR);
    } catch (const vk::OutOfDateKHRError &) {
        presentResult = vk::Result::eErrorOutOfDateKHR;
    }

    // Due to VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS being defined, eErrorOutOfDateKHR can be checked as a result
    // here and does not need to be caught by an exception.
    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR ||
        externalResize
    ) {
        swapChain.recreate();
    } else if (presentResult != vk::Result::eSuccess) {
        // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
        throw std::runtime_error("failed to present");
    }

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::createCommandPool() {
    const vk::CommandPoolCreateInfo commandPoolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = device.queueFamilyIndex()
    };
    commandPool = vk::raii::CommandPool(device.logical(), commandPoolInfo);
}

void Renderer::createCommandBuffers() {
    commandBuffers.clear();
    const vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    commandBuffers = vk::raii::CommandBuffers(device.logical(), allocInfo);
}

// Create all semaphores to sync rendering
void Renderer::createSyncObjects() {
    assert(
        presentCompleteSemaphores.empty()
        && renderFinishedSemaphores.empty()
        && inFlightFences.empty()
    );

    // One render-finished semaphore per swapchain image (indexed by imageIndex)
    for (size_t i = 0; i < swapChain.imageCount(); i++) {
        renderFinishedSemaphores.emplace_back(device.logical(), vk::SemaphoreCreateInfo{});
    }

    // One present-complete semaphore and one fence per frame in flight
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        presentCompleteSemaphores.emplace_back(device.logical(), vk::SemaphoreCreateInfo{});
        inFlightFences.emplace_back(
            device.logical(),
            vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}
        );
    }
}

// Transition image layout to one that's suitable for rendering.
void Renderer::transitionImageLayout(
    const uint32_t imageIndex,
    const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout,
    const vk::AccessFlags2 srcAccessMask, const vk::AccessFlags2 dstAccessMask,
    const vk::PipelineStageFlags2 srcStageMask, const vk::PipelineStageFlags2 dstStageMask
) const {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapChain.image(imageIndex),
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    const vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
}

// Write commands we want to execute into a command buffer
void Renderer::recordCommandBuffer(const uint32_t imageIndex) const {
    const auto &commandBuffer = commandBuffers[frameIndex];
    commandBuffer.begin({});

    // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    transitionImageLayout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {}, // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    // Set up the color attachment
    constexpr vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    constexpr vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderingAttachmentInfo colorAttachment = {
        .imageView = swapChain.imageView(imageIndex),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingAttachmentInfo depthAttachment = {
        .imageView = swapChain.depthView(),
        .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth
    };

    // Set up the rendering info
    const vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0, 0}, .extent = swapChain.extent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = &depthAttachment
    };

    // Begin rendering
    commandBuffer.beginRendering(renderingInfo);

    // Rendering commands will go here
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle());

    // bind the vertex buffer at binding 0 with offset 0
    constexpr vk::DeviceSize offset = 0;
    commandBuffer.bindVertexBuffers(0, *vertexBuffer.handle(), offset);
    // bind the index buffer
    commandBuffer.bindIndexBuffer(*indexBuffer.handle(), 0, vk::IndexType::eUint16);

    // Bind this frame's descriptor set so the shader can find its uniform buffer
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipeline.layout(),
        0, // first set
        *descriptorSets[frameIndex],
        nullptr // dynamic offsets, unused
    );

    const auto extent = swapChain.extent();
    commandBuffer.setViewport(
        0,
        vk::Viewport(
            0.0f, 0.0f,
            static_cast<float>(extent.width),
            static_cast<float>(extent.height),
            0.0f, 1.0f
        )
    );
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));

    // Read the count from the buffer.
    commandBuffer.drawIndexed(indexCount, 1, 0, 0, 0);

    // End rendering
    commandBuffer.endRendering();

    // After rendering, transition the swap chain image to PRESENT_SRC
    transitionImageLayout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    commandBuffer.end();
}

void Renderer::updateUniformBuffer(const uint32_t frameIdx) const {
    // Time since program start, used to drive the rotation
    static auto startTime = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float, std::chrono::seconds::period>(
        currentTime - startTime).count();

    const auto [width, height] = swapChain.extent();

    UniformBufferObject ubo{};

    // Spin around the Z axis at 90 degrees per second
    ubo.model = glm::rotate(
        glm::mat4(1.0f),
        time * glm::radians(90.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    // Camera at (2,2,2) looking at the origin, with +Z as up
    ubo.view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    // 45-degree perspective, aspect ratio derived from the current swapchain
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(width) /
        static_cast<float>(height),
        0.1f,
        10.0f);

    // GLM was designed for OpenGL; Vulkan's Y is flipped relative to that
    ubo.proj[1][1] *= -1;

    // Write directly into the persistently-mapped buffer for this frame
    std::memcpy(uniformBuffersMapped[frameIdx], &ubo, sizeof(ubo));
}

void Renderer::createDescriptorPool() {
    // Two pool sizes now: one for UBOs (matrices), one for combined image samplers (textures)
    std::array<vk::DescriptorPoolSize, 2> poolSizes{
        {
            {.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = MAX_FRAMES_IN_FLIGHT},
            {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = MAX_FRAMES_IN_FLIGHT}
        }
    };

    const vk::DescriptorPoolCreateInfo poolInfo{
        // Required for vk::raii::DescriptorSet's destructor
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        // maxSets caps how many descriptor *sets* (not individual descriptors) can be allocated from this pool over its lifetime
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    descriptorPool = vk::raii::DescriptorPool(device.logical(), poolInfo);
}


void Renderer::createDescriptorSets() {
    // allocateDescriptorSets wants one layout pointer per set, even though they're all the same layout in our case
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *pipeline.descriptorLayout());
    const vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    descriptorSets.clear();
    descriptorSets = device.logical().allocateDescriptorSets(allocInfo);

    // Each descriptor set is allocated but empty — point each one at its corresponding uniform buffer
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers[i].handle(),
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        // Combined image sampler bundles both the image view and sampler into one descriptor
        vk::DescriptorImageInfo imageInfo{
            .sampler = textureSampler->handle(),
            .imageView = textureImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        // Two writes: binding 0 is the UBO (matrices), binding 1 is the texture
        std::array<vk::WriteDescriptorSet, 2> writes{
            {
                {
                    .dstSet = descriptorSets[i],
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo = &bufferInfo
                },
                {
                    .dstSet = descriptorSets[i],
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                    .pImageInfo = &imageInfo
                }
            }
        };

        device.logical().updateDescriptorSets(writes, {});
    }
}

void Renderer::createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(
        "textures/texture2.jpg", &texWidth, &texHeight, &texChannels,
        STBI_rgb_alpha // force an alpha channel even if it doesn't have one
    );
    const vk::DeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    // Stage on host-visible memory first
    Buffer staging(
        device,
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    staging.uploadData(pixels, imageSize);

    stbi_image_free(pixels);

    // Create the device-local image
    textureImage.emplace(
        device,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight),
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    // Move it to a layout that can receive a transfer, copy, then move to a layout suitable for shader sampling
    textureImage->transitionLayout(
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal
    );
    textureImage->copyFromBuffer(
        staging.handle(),
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );
    textureImage->transitionLayout(
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

    // View lets shaders access the image
    textureImageView = textureImage->createView();
    // sampler defines how it's filtered
    textureSampler.emplace(device);

    // staging automatically destroys here
}
