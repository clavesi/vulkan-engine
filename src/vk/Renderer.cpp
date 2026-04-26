#include "Renderer.h"
#include "Device.h"
#include "SwapChain.h"
#include "Pipeline.h"
#include "core/Vertex.h"

#include <cassert>
#include <stdexcept>


namespace {
    // top left = = red, top right = green, bottom right = blue, bottom left = white
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    // Index buffers allows you to reorder the vertex buffer, allowing reuse of data for multiple vertex.
    // For example, a rectangle, split into two triangles, would have six vertices. But they share the two on the diagonal.
    // With index buffer, we only need to define them once in the vertex buffer and then use it twice in the index buffer.
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
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

    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void Renderer::drawFrame(bool externalResize) {
    // Note: inFlightFences, presentCompleteSemaphores, and commandBuffers are indexed by frameIndex,
    //       while renderFinishedSemaphores is indexed by imageIndex
    auto fenceResult = device.logical().waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
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
    vk::CommandPoolCreateInfo commandPoolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = device.queueFamilyIndex()
    };
    commandPool = vk::raii::CommandPool(device.logical(), commandPoolInfo);
}

void Renderer::createCommandBuffers() {
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{
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
    uint32_t imageIndex,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask
) {
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

    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
}

// Write commands we want to execute into a command buffer
void Renderer::recordCommandBuffer(uint32_t imageIndex) {
    auto &commandBuffer = commandBuffers[frameIndex];
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
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = swapChain.imageView(imageIndex),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    // Set up the rendering info
    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0, 0}, .extent = swapChain.extent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo
    };

    // Begin rendering
    commandBuffer.beginRendering(renderingInfo);

    // Rendering commands will go here
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle());

    // bind the vertex buffer at binding 0 with offset 0
    vk::DeviceSize offset = 0;
    commandBuffer.bindVertexBuffers(0, *vertexBuffer.handle(), offset);
    // bind the index buffer
    commandBuffer.bindIndexBuffer(*indexBuffer.handle(), 0, vk::IndexType::eUint16);

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
