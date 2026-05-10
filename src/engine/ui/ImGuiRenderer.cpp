#include "ImGuiRenderer.h"

#include "vk/Device.h"
#include "vk/SwapChain.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

static void check_vk_result(VkResult err) {
    if (err != VK_SUCCESS) {
        throw std::runtime_error("ImGui Vulkan error: " + std::to_string(err));
    }
}

ImGuiRenderer::ImGuiRenderer(const Device &device, SwapChain &swapChain,
                             const vk::raii::Instance &instance,
                             GLFWwindow *window)
    : device(device) {
    VkDescriptorPoolSize pool_size = {
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 8;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    VkDescriptorPool rawPool;
    VkResult result = vkCreateDescriptorPool(*device.logical(), &pool_info, nullptr, &rawPool);
    check_vk_result(result);
    imguiPool = vk::raii::DescriptorPool(device.logical(), rawPool);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = *instance;
    init_info.PhysicalDevice = *device.physical();
    init_info.Device = *device.logical();
    init_info.QueueFamily = device.queueFamilyIndex();
    init_info.Queue = *device.graphicsQueue();
    init_info.DescriptorPool = *imguiPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = static_cast<uint32_t>(swapChain.imageCount());
    init_info.PipelineInfoMain.MSAASamples = static_cast<VkSampleCountFlagBits>(static_cast<uint32_t>(swapChain.
        samples()));
    init_info.CheckVkResultFn = check_vk_result;
    init_info.UseDynamicRendering = true;

    colorFormat = static_cast<VkFormat>(swapChain.format());
    depthFormat = static_cast<VkFormat>(swapChain.depthFormat());
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;

    ImGui_ImplVulkan_Init(&init_info);

    // Upload ImGui font texture
    auto cmd = device.beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(*cmd);
    device.endSingleTimeCommands(cmd);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

ImGuiRenderer::~ImGuiRenderer() {
    device.waitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiRenderer::initGlfw(GLFWwindow *window) {
    ImGui_ImplGlfw_InitForVulkan(window, true);
}

void ImGuiRenderer::beginFrame() const {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiRenderer::render(const vk::raii::CommandBuffer &cmd) const {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
}
