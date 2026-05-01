#pragma once

#include <string_view>
#include <utility>
#include <vulkan/vulkan_raii.hpp>

// Forward-declare GLFW so it's kept out of the header and not pulled in everywhere.
struct GLFWwindow;

// Forward-declare vulkan_raii types we need in the interface
namespace vk::raii {
    class Instance;
    class SurfacekHR;
}

class Window {
public:
    Window(uint32_t width, uint32_t height, std::string_view title);

    ~Window();

    // Only one window can exist. Non-copyable, non-movable - owns a raw GLFW handle
    Window(const Window &) = delete;

    Window &operator=(const Window &) = delete;

    Window(Window &&) = delete;

    Window operator=(Window &&) = delete;

    [[nodiscard]] bool shouldClose() const;

    static void pollEvents() ;

    // Returs {width, height} in pixels.
    // Blocks (waiting on events) while the window is minimized.
    [[nodiscard]] std::pair<int, int> getFramebufferSize() const;

    void waitWhileMinimized() const;

    [[nodiscard]] bool wasResized() const { return framebufferResized; }
    void resetResizedFlag() { framebufferResized = false; }

    // Creates a VkSurfaceKHR for this window on the given instance
    [[nodiscard]] vk::raii::SurfaceKHR createSurface(const vk::raii::Instance &instance) const;

private:
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

    GLFWwindow *handle = nullptr;
    bool framebufferResized = false;
};
