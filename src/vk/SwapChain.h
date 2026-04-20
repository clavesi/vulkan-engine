#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>

class Window;
class Device;

class SwapChain {
public:
    SwapChain(const Device &device, const Window &window, const vk::raii::SurfaceKHR &surface);

    SwapChain(const SwapChain &) = delete;
    SwapChain &operator=(const SwapChain &) = delete;

    // Tears down and rebuilds the swap chain. Blocks while the window is
    // minimized, and waits for the device to be idle before destroying the
    // old resources. Safe to call at any time after construction.
    // FORMERLY: recreateSwapChain();
    void recreate();

    [[nodiscard]] const vk::raii::SwapchainKHR &handle() const { return swapChain; }
    [[nodiscard]] vk::Format format() const { return surfaceFormat.format; }
    [[nodiscard]] vk::Extent2D extent() const { return swapChainExtent; }
    [[nodiscard]] const std::vector<vk::Image> &images() const { return swapChainImages; }
    [[nodiscard]] const std::vector<vk::raii::ImageView> &imageViews() const { return swapChainImageViews; }
    [[nodiscard]] size_t imageCount() const { return swapChainImages.size(); }
    [[nodiscard]] vk::Image image(size_t i) const { return swapChainImages[i]; }
    [[nodiscard]] const vk::raii::ImageView &imageView(size_t i) const { return swapChainImageViews[i]; }

private:
    // FORMERLY: createSwapChain()
    void create();
    void createImageViews();
    // FORMERLY: cleanupSwapChain()
    void destroy();

    static uint32_t chooseMinImageCount(const vk::SurfaceCapabilitiesKHR &caps);
    static vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &available);
    static vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR> &available);
    vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR &caps) const;

    // Stored references — these outlive the SwapChain because the Engine
    // destroys its members in reverse declaration order.
    const Device &device;
    const Window &window;
    const vk::raii::SurfaceKHR &surface;

    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::raii::ImageView> swapChainImageViews;
};
