#include "Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan_raii.hpp>
#include <stdexcept>

Window::Window(const uint32_t width, const uint32_t height, const std::string_view title) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    handle = glfwCreateWindow(
        static_cast<int>(width),
        static_cast<int>(height),
        title.data(),
        nullptr, nullptr
    );
    if (!handle) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(handle, this);
    glfwSetFramebufferSizeCallback(handle, framebufferResizeCallback);
}

Window::~Window() {
    if (handle) {
        glfwDestroyWindow(handle);
    }
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(handle);
}

void Window::pollEvents() {
    glfwPollEvents();
}

std::pair<int, int> Window::getFramebufferSize() const {
    int width = 0, height = 0;
    glfwGetFramebufferSize(handle, &width, &height);
    return {width, height};
}

void Window::waitWhileMinimized() const {
    int width = 0, height = 0;
    glfwGetFramebufferSize(handle, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(handle, &width, &height);
        glfwWaitEvents();
    }
}

vk::raii::SurfaceKHR Window::createSurface(const vk::raii::Instance &instance) const {
    // This is more complicated if we're not using a library, but since we are, this is very simple
    VkSurfaceKHR raw;
    if (glfwCreateWindowSurface(*instance, handle, nullptr, &raw) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }
    return vk::raii::SurfaceKHR(instance, raw);
}

void Window::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    self->framebufferResized = true;
}
