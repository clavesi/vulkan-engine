#pragma once

#include <vulkan/vulkan_raii.hpp>

struct GLFWwindow;

class Device;
class SwapChain;

class ImGuiRenderer {
public:
    ImGuiRenderer(const Device &device, SwapChain &swapChain, const vk::raii::Instance &instance, GLFWwindow *window);
    ~ImGuiRenderer();

    ImGuiRenderer(const ImGuiRenderer &) = delete;
    ImGuiRenderer &operator=(const ImGuiRenderer &) = delete;

    void initGlfw(GLFWwindow* window);

    void beginFrame() const;
    void render(const vk::raii::CommandBuffer &cmd) const;

private:
    const Device &device;
    vk::raii::DescriptorPool imguiPool = nullptr;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
};
