#include "SwapChain.h"
#include "Device.h"
#include "../core/Window.h"

SwapChain::SwapChain(const Device &device,
                     const Window &window,
                     const vk::raii::SurfaceKHR &surface)
    : device(device), window(window), surface(surface) {
    chosenDepthFormat = device.findDepthFormat();
    create();
    createDepthResources();
}

// Recreate the swap chain. Useful for things like the window is resized or minimized.
void SwapChain::recreate() {
    // If the window is minimized, block until it's restored.
    window.waitWhileMinimized();
    // don't touch resources that may still be in use
    device.waitIdle();

    destroy();
    create();
    createDepthResources();
}

void SwapChain::create() {
    const vk::raii::PhysicalDevice &physical = device.physical();

    // Get basic surface capabilities
    const vk::SurfaceCapabilitiesKHR surfaceCaps = physical.getSurfaceCapabilitiesKHR(surface);
    swapChainExtent = chooseExtent(surfaceCaps);

    // Get supported surface formats
    const std::vector<vk::SurfaceFormatKHR> formats = physical.getSurfaceFormatsKHR(surface);
    surfaceFormat = chooseSurfaceFormat(formats);

    // Get supported presentation modes
    const std::vector<vk::PresentModeKHR> presentModes = physical.getSurfacePresentModesKHR(surface);
    const vk::PresentModeKHR presentMode = choosePresentMode(presentModes);
    const uint32_t minImageCount = chooseMinImageCount(surfaceCaps);

    const vk::SwapchainCreateInfoKHR createInfo{
        .surface = surface,
        .minImageCount = minImageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        // Specify the number of layers of each image. Typically just 1 unless doing stereoscopic 3D apps
        .imageArrayLayers = 1,
        // Specify what kind of operations we'll use the images in the swap chain for
        // We're rendering directly to them, so eColorAttachment
        // If you first do post-processing, use something like eTransferDst
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        // Specifies how to handle swap chain images that might be used across multiple queue families.
        // eExclusive: image owned by one family at a time and ownership must be explicitly transferred before using
        // eConcurrent: can be used across families without explicit ownership transfers. Requires specifying which queue families will be used with additional parameters.
        .imageSharingMode = vk::SharingMode::eExclusive,
        // Specify certain transform should be applied to images like rotations or flips. currentTransform means no transforms.
        .preTransform = surfaceCaps.currentTransform,
        // Specifies if the alpha channel should be used for blending with other windows in the window system
        // Usually ignore alpha, so just eOpaque
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = true
    };

    swapChain = vk::raii::SwapchainKHR(device.logical(), createInfo);
    swapChainImages = swapChain.getImages();

    vk::ImageViewCreateInfo viewInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = surfaceFormat.format,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    };

    for (const auto &image: swapChainImages) {
        viewInfo.image = image;
        swapChainImageViews.emplace_back(device.logical(), viewInfo);
    }
}

void SwapChain::destroy() {
    depthImageView = nullptr;
    depthImage.reset();

    // Clear the image views before destroying the swap chain since they depend on the swap chain images.
    swapChainImageViews.clear();
    swapChain = nullptr;
}

uint32_t SwapChain::chooseMinImageCount(const vk::SurfaceCapabilitiesKHR &caps) {
    auto minImageCount = std::max(3u, caps.minImageCount);
    if ((caps.maxImageCount > 0) && (caps.maxImageCount < minImageCount)) {
        minImageCount = caps.maxImageCount;
    }
    return minImageCount;
}

/*
 * Determine the correct surface format (color depth)
 */
vk::SurfaceFormatKHR SwapChain::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &available) {
    // Each SurfaceFormatKHR contains a `format` and `colorSpace` member.
    // `format` specifies the color channels, and types
    // `colorSpace` indicates if the SRGB color space is supported or not
    assert(!available.empty());

    // Use SRGB if available
    const auto iter = std::ranges::find_if(
        available,
        [](const auto &format) {
            return format.format == vk::Format::eB8G8R8A8Srgb &&
                   format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        }
    );

    // If not available, just use the first format specified
    return iter != available.end() ? *iter : available[0];
}

/*
  Determine the correct presentation mode (condition for "swapping" images to the screen)
  Four options:
  - eImmediate: Images submitted by your app are transferred to the screen right away. May result in tearing.
  - eFifo: Swap chain is a queue where the display takes from front when refreshed and program inserts rendered images at the back. If queue is full, then program has to wait. This is like vsync.
  - eFifoRelaxed: Same as previous, but if the application is late and the queue was empty at the last vertical blank, then instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives. Results in tearing.
  - eMailbox: Another version of eFifo. Instead of blocking the app when queue is full, the images that are in queue are replaced with newer ones. Renders frame faster while still avoiding tearing. Thus, it has fewer latency issues. Also called "triple buffering".
 */
vk::PresentModeKHR SwapChain::choosePresentMode(const std::vector<vk::PresentModeKHR> &available) {
    // eFifo is the only guaranteed available mode
    assert(std::ranges::any_of(
        available,
        [](auto mode) {return mode==vk::PresentModeKHR::eFifo;}));

    const bool hasMailbox = std::ranges::any_of(
        available,
        [](auto mode) { return mode == vk::PresentModeKHR::eMailbox; });

    // Pick eMailbox if available, otherwise default to eFifo.
    return hasMailbox ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}

/*
Determine the correct swap extent (resolution of images in the swap chain)
Typically, this resolution is the same as the window, but some window managers allow you to set them separately.
*/
vk::Extent2D SwapChain::chooseExtent(const vk::SurfaceCapabilitiesKHR &caps) const {
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return caps.currentExtent;
    }

    // Query window resolution in pixels before matching it against the min and max image extent
    auto [width, height] = window.getFramebufferSize();
    return {
        std::clamp<uint32_t>(width, caps.minImageExtent.width, caps.maxImageExtent.width),
        std::clamp<uint32_t>(height, caps.minImageExtent.height, caps.maxImageExtent.height)
    };
}

void SwapChain::createDepthResources() {
    depthImage.emplace(
        device,
        swapChainExtent.width,
        swapChainExtent.height,
        chosenDepthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    depthImageView = depthImage->createView(vk::ImageAspectFlagBits::eDepth);

    // Transition once — depth image is reused every frame, so this layout
    // persists across frames (we'll re-transition each frame in the renderer
    // because the load op clears anyway, but doing it here primes the layout)
    depthImage->transitionLayout(
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal);
}
