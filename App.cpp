#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

// If not using a library like GLFW, make sure to import proper platform-specific extensions for surface creation.
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/00_Window_surface.html#_window_surface_creation
#define GLFW_INCLUDE_VULKAN // REQUIRED only for GLFW CreateWindowSurface.
#include <GLFW/glfw3.h>


constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

// What validation layers
const std::vector<char const *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
// and whether to enable them
#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *window = nullptr;

    // Data member to hold the handle to the instance and raii context
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;

    // Set up callback function for debug messaging
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;

    // Use the Window System Integration (WSI) to create a surface to present rendered images to
    // Needs to be created right after the instance creation since it can influence the physical device selection.
    vk::raii::SurfaceKHR surface = nullptr;

    // Store graphics card
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    std::vector<const char *> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName
    };

    // Store logical device handle and features
    vk::raii::Device device = nullptr;
    vk::PhysicalDeviceFeatures deviceFeatures; // currently unused. used in later sections
    // Store graphics queue family handle
    vk::raii::Queue graphicsQueue = nullptr;

    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::raii::ImageView> swapChainImageViews;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        // Create instance, the connection between the app and the Vulkan lib
        createInstance();
        setupDebugMessenger();
        createSurface();
        // Pick graphics card(s)
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
    }

    void createInstance() {
        vk::ApplicationInfo appInfo{};
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = vk::ApiVersion14;

        // Get the required layers
        std::vector<const char *> requiredLayers;
        if (enableValidationLayers) {
            requiredLayers.assign(validationLayers.begin(), validationLayers.end());
        }

        // Check if the required layers are supported by the Vulkan implementation
        auto layerProperties = context.enumerateInstanceLayerProperties();
        const auto unsupportedLayerIt = std::ranges::find_if(requiredLayers,
                                                             [&layerProperties](auto const &requiredLayer) {
                                                                 return std::ranges::none_of(layerProperties,
                                                                     [requiredLayer](auto const &layerProperty) {
                                                                         return strcmp(layerProperty.layerName,
                                                                                 requiredLayer) == 0;
                                                                     });
                                                             });
        if (unsupportedLayerIt != requiredLayers.end()) {
            throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
        }

        // Get the required extensions
        auto requiredExtensions = getRequiredInstanceExtensions();

        // Check if the required extensions are supported by the Vulkan implementation
        auto extensionProperties = context.enumerateInstanceExtensionProperties();
        const auto unsupportedPropertyIt = std::ranges::find_if(requiredExtensions,
                                                                [&extensionProperties](auto const &requiredExtension) {
                                                                    return std::ranges::none_of(extensionProperties,
                                                                        [requiredExtension](
                                                                    auto const &extensionProperty) {
                                                                            return strcmp(
                                                                                    extensionProperty.extensionName,
                                                                                    requiredExtension) == 0;
                                                                        });
                                                                });
        if (unsupportedPropertyIt != requiredExtensions.end()) {
            throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
        }

        vk::InstanceCreateInfo createInfo{};
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
        createInfo.ppEnabledLayerNames = requiredLayers.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        // Create instance
        instance = vk::raii::Instance(context, createInfo);
    }

    void mainLoop() const {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() const {
        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void setupDebugMessenger() {
        if constexpr (!enableValidationLayers) return;

        // Fill in struct with details about the messenger and its callback
        constexpr vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        constexpr vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT;
        debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
        debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
        // Pointer to callback function
        debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugCallback;
        // Optionally, pass a pointer to the pUserData field which will be passed to the callback function via that parameter
        // Use this, for example, to pass a pointer to the mai class.

        debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    void createSurface() {
        // Again, this is more complicated if we're not using a library, but since we are, this is very simple
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
            throw std::runtime_error("failed to create window surface!");
        }
        surface = vk::raii::SurfaceKHR(instance, _surface);
    }

    bool isDeviceSuitable(vk::raii::PhysicalDevice const &physical_device) {
        // Check if the physicalDevice supports the Vulkan 1.3 API version
        bool supportsVulkan1_3 = physical_device.getProperties().apiVersion >= vk::ApiVersion13;

        // Check if any of the queue families support graphics operations
        auto queueFamilies = physical_device.getQueueFamilyProperties();
        bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const &qfp) {
            return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
        });

        // Check if all required physicalDevice extensions are available
        auto availableDeviceExtensions = physical_device.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions =
                std::ranges::all_of(requiredDeviceExtension,
                                    [&availableDeviceExtensions](auto const &requiredDeviceExtension) {
                                        return std::ranges::any_of(availableDeviceExtensions,
                                                                   [requiredDeviceExtension](
                                                               auto const &availableDeviceExtension) {
                                                                       return strcmp(
                                                                                  availableDeviceExtension.
                                                                                  extensionName,
                                                                                  requiredDeviceExtension) == 0;
                                                                   });
                                    });

        // Check if the physicalDevice supports the required features (dynamic rendering and extended dynamic state)
        auto features =
                physical_device
                .getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
                    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportsRequiredFeatures = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                        features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().
                                        extendedDynamicState;

        // Return true if the physicalDevice meets all the criteria
        return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
    }

    void pickPhysicalDevice() {
        std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
        auto const devIter = std::ranges::find_if(physicalDevices, [&](auto const &pd) {
            return isDeviceSuitable(pd);
        });
        if (devIter == physicalDevices.end()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        physicalDevice = *devIter;
    }

    void createLogicalDevice() {
        // Find index of the first queue family
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();


        // Get the first index into that queue family that supports graphics and present
        uint32_t queueIndex = ~0;
        for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
            if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
                // found a queue family that supports both graphics and present
                queueIndex = qfpIndex;
                break;
            }
        }
        if (queueIndex == ~0) {
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        }

        // Query for 1.3 features
        // Structure chain automatically connects these structures together by setting up pNext pointers between them
        // Default-construct the chain and then set the feature fields explicitly. Initializer-list / designated
        // initializers don't match StructureChain's constructors (clangd/clang error), so assign members via .get<T>().
        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain{};
        // Leave PhysicalDeviceFeatures2 empty
        // Enable dynamic rendering from Vulkan 1.3
        featureChain.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = VK_TRUE;
        // Enable extended dynamic state from the extension
        featureChain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState = VK_TRUE;

        // Create a device
        float queuePriority = 0.5f; // influence scheduling of command buffer (0.0-1.0)
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
        deviceQueueCreateInfo.queueFamilyIndex = queueIndex;
        deviceQueueCreateInfo.queueCount = 1;
        deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size());
        deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

        device = vk::raii::Device(physicalDevice, deviceCreateInfo);
        graphicsQueue = vk::raii::Queue(device, queueIndex, 0);
    }

    void createSwapChain() {
        // Get basic surface capabilities
        const vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
        swapChainExtent = chooseSwapExtent(surfaceCapabilities);
        const uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

        // Get supported surface formats
        const std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(surface);
        swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

        // Get supported presentation modes
        const std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);
        const vk::PresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes);

        vk::SwapchainCreateInfoKHR swapChainCreateInfo;
        swapChainCreateInfo.surface = *surface;
        swapChainCreateInfo.minImageCount = minImageCount;
        swapChainCreateInfo.imageFormat = swapChainSurfaceFormat.format;
        swapChainCreateInfo.imageColorSpace = swapChainSurfaceFormat.colorSpace;
        swapChainCreateInfo.imageExtent = swapChainExtent;
        // Specify the number of layers of each image. Typically just 1 unless doing stereoscopic 3D apps
        swapChainCreateInfo.imageArrayLayers = 1;
        // Specify what kind of operations we'll use the images in the swap chain for
        // We're rendering directly to them, so eColorAttachment
        // If you first do post-processing, use something like eTransferDst
        swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        // Specifies how to handle swap chain images that might be used across multiple queue families.
        // eExclusive: image owned by one family at a time and ownership must be explicitly transferred before using
        // eConcurrent: can be used across families without explicit ownership transfers. Requires specifying which queue families will be used with additional parameters.
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        // Specify certain transform should be applied to images like rotations or flips. currentTransform means no transforms.
        swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        // Specifies if the alpha channel should be used for blending with other windows in the window system
        // Usually ignore alpha, so just eOpaque
        swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCreateInfo.presentMode = presentMode;
        swapChainCreateInfo.clipped = true;

        swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
        swapChainImages = swapChain.getImages();
    }

    void createImageViews() {
        assert(swapChainImages.emplace_back());

        vk::ImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imageViewCreateInfo.format = swapChainSurfaceFormat.format;
        imageViewCreateInfo.subresourceRange = {
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
        };

        for (auto &image: swapChainImages) {
            imageViewCreateInfo.image = image;
            swapChainImageViews.emplace_back(device, imageViewCreateInfo);
        }
    }

    static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities) {
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
        if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount)) {
            minImageCount = surfaceCapabilities.maxImageCount;
        }
        return minImageCount;
    }

    /*
     * Determine the correct surface format (color depth)
     */
    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats) {
        // Each SurfaceFormatKHR contains a `format` and `colorSpace` member.
        // `format` specifies the colors channels and types
        // `colorSpace` indicates if the SRGB color space is supported or not
        assert(!availableFormats.empty());

        // Use SRGB if available
        const auto formatIt = std::ranges::find_if(
            availableFormats,
            [](const auto &format) {
                return format.format == vk::Format::eB8G8R8A8Srgb &&
                       format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
            }
        );
        // If not available, just use the first format specified
        return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
    }

    /*
      Determine the correct presentation mode (condition for "swapping" images to the screen)
      Four options:
      - eImmediate: Images submitted by your app are transferred to the screen right away. May result in tearing.
      - eFifo: Swap chain is a queue where the display takes from front when refreshed and program inserts rendered images at the back. If queue is full, then program has to wait. This is like vsync.
      - eFifoRelaxed: Same as previous, but if the application is late and the queue was empty at the last vertical blank, then instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives. Results in tearing.
      - eMailbox: Another version of eFifo. Instead of blocking the app when queue is full, the images that are in queue are replaced with newer ones. Renders frame faster while still avoiding tearing. Thus, it has fewer latency issues. Also called "triple buffering".
     */
    static vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes) {
        // eFifo is the only guaranteed available mode
        assert(std::ranges::any_of(
                availablePresentModes,
                [](auto presentMode) {return presentMode==vk::PresentModeKHR::eFifo;})
        );
        // Pick eMailbox if available, otherwise default to eFifo.
        return std::ranges::any_of(
                   availablePresentModes,
                   [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }
               )
                   ? vk::PresentModeKHR::eMailbox
                   : vk::PresentModeKHR::eFifo;
    }

    /*
    Determine the correct swap extend (resolution of images in swap chain)
    Typically this resolution is the same as the window, but some window managers allow you to set them separately.
   */
    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        // Query window resolution in pixels before matching it against the min and max image extent
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        return {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    // Returns list of the required instance extensions
    static std::vector<const char *> getRequiredInstanceExtensions() {
        uint32_t glfwExtensionCount = 0;
        // extensions specified by GLFW are always required when using it for windowing
        const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (enableValidationLayers) {
            // debug messenger extension
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        return extensions;
    }

    /*
     * INPUT:
     * severity: flag that specifies severity of message (eVerbose, eInfo, eWarning, eError)
     *          setup so you can check if a message is equal or worse than another
     *          i.e. severity >= ...::eWarning {} will show both warning and error messages
     * type: flag for whether message relates to Vulkan specs or not (eGeneral, eValidation, ePerformance)
     * pCallbackData: struct with message details like the debug message and Vulkan object handles related to it
     * pUserData: pointer specified during setup of the callback and allows passing in your own data
     *
     * OUTPUT:
     * Boolean that indicates if the Vulkan call that triggered the validation layer message should be aborted.
     */
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                          const vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                          void *pUserData) {
        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

        return vk::False;
    }
};

int main() {
    try {
        HelloTriangleApplication app;
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
