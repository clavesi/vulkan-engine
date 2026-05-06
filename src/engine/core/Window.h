#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

#include <string_view>
#include <utility>

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

    static void pollEvents();

    // Returns {width, height} in pixels.
    // Blocks (waiting on events) while the window is minimized.
    [[nodiscard]] std::pair<int, int> getFramebufferSize() const;

    void waitWhileMinimized() const;

    [[nodiscard]] bool wasResized() const { return framebufferResized; }
    void resetResizedFlag() { framebufferResized = false; }

    // Creates a VkSurfaceKHR for this window on the given instance
    [[nodiscard]] vk::raii::SurfaceKHR createSurface(const vk::raii::Instance &instance) const;

    // Returns mouse movement since last resetFrameInput() call,
    // only when right mouse button is held
    glm::vec2 getMouseDelta() const { return mouseDelta; }
    // Returns scroll wheel movement since last resetFrameInput() call
    float getScrollDelta() const { return scrollDelta; }
    // Call once per frame after consuming input to clear per-frame accumulators
    void resetFrameInput();

private:
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

    // GLFW callback targets — static because GLFW is C
    static void mouseMoveCallback(GLFWwindow* window, double x, double y);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

    GLFWwindow *handle = nullptr;
    bool framebufferResized = false;

    // Per-frame input accumulators — reset each frame
    glm::vec2 mouseDelta = {0.0f, 0.0f};
    float scrollDelta = 0.0f;
    // Track right mouse button state for drag detection
    bool rightMouseHeld = false;
    glm::vec2 lastMousePos = {0.0f, 0.0f};


};
