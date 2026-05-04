#include "io/ModelLoader.h"
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

Renderer::Renderer(const Device &device, SwapChain &swapChain, const Pipeline &pipeline, const EngineConfig &config)
    : device(device),
      swapChain(swapChain),
      pipeline(pipeline),
      config(config) {
    // Load model from disk and upload to GPU
    auto [vertices, indices] = io::loadObj(config.modelPath);
    mesh.emplace(device, vertices, indices);

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

void Renderer::drawFrame(const glm::mat4 view, const glm::mat4 proj, const bool externalResize) {
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
    updateUniformBuffer(frameIndex, view, proj);

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
    vk::Image image,
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
        .image = image,
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
        swapChain.image(imageIndex),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {}, // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );
    transitionImageLayout(
        swapChain.colorImageHandle(),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    // Set up the color attachment
    constexpr vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    constexpr vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderingAttachmentInfo colorAttachment = {
        .imageView = *swapChain.colorView(), // multisampled — what we draw INTO
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eAverage, // average the N samples per pixel
        .resolveImageView = swapChain.imageView(imageIndex), // single-sampled — where the resolve goes
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore, // store the resolved single-sample result
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
    commandBuffer.bindVertexBuffers(0, *mesh->vertexBuffer().handle(), offset);
    // bind the index buffer
    commandBuffer.bindIndexBuffer(*mesh->indexBuffer().handle(), 0, vk::IndexType::eUint32);

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
    commandBuffer.drawIndexed(mesh->indexCount(), 1, 0, 0, 0);

    // End rendering
    commandBuffer.endRendering();

    // After rendering, transition the swap chain image to PRESENT_SRC
    transitionImageLayout(
        swapChain.image(imageIndex),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    commandBuffer.end();
}

void Renderer::updateUniformBuffer(const uint32_t frameIdx, const glm::mat4 view, const glm::mat4 proj) const {
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = view;
    ubo.proj = proj;

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
        config.texturePath.c_str(), &texWidth, &texHeight, &texChannels,
        STBI_rgb_alpha // force an alpha channel even if it doesn't have one
    );
    const vk::DeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    // floor(log_2(max_dim)) + 1 - number of mip levels down to 1x1
    const uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    // Stage on host-visible memory first
    const Buffer staging(
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
        mipLevels,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
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
    textureImage->generateMipmaps(vk::Format::eR8G8B8A8Srgb, texWidth, texHeight);

    // View lets shaders access the image
    textureImageView = textureImage->createView();
    // sampler defines how it's filtered
    textureSampler.emplace(device, vk::LodClampNone);

    // staging automatically destroys here
}
