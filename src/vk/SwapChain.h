#pragma once

#include "Image.h"

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <optional>

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
    [[nodiscard]] vk::Image image(const size_t i) const { return swapChainImages[i]; }
    [[nodiscard]] const vk::raii::ImageView &imageView(const size_t i) const { return swapChainImageViews[i]; }

    [[nodiscard]] vk::Format depthFormat() const { return chosenDepthFormat; }
    [[nodiscard]] const vk::raii::ImageView &depthView() const { return depthImageView; }

    vk::Image colorImageHandle() const { return *colorImage->handle(); }
    vk::raii::ImageView &colorView() { return colorImageView; }
    vk::SampleCountFlagBits samples() const { return msaaSamples; }

private:
    // FORMERLY: createSwapChain()
    void create();
    // FORMERLY: cleanupSwapChain()
    void destroy();

    static uint32_t chooseMinImageCount(const vk::SurfaceCapabilitiesKHR &caps);
    static vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &available);
    static vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR> &available);
    [[nodiscard]] vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR &caps) const;

    void createDepthResources();
    void createColorResources();

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

    vk::Format chosenDepthFormat;
    std::optional<Image> depthImage;
    vk::raii::ImageView depthImageView = nullptr;

    // MSAA color attachment — what the pipeline draws into.
    // Resolved into the swapchain image at end-of-frame.
    std::optional<Image> colorImage;
    vk::raii::ImageView colorImageView = nullptr;
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
};
