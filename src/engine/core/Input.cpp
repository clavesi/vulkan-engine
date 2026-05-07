#include "Input.h"

void Input::init(GLFWwindow *window) {
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    // Initialise mouse position to current cursor pos
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    mousePosition = {static_cast<float>(x), static_cast<float>(y)};
    lastPosition = mousePosition;
}

void Input::reset() {
    mouseDelta = {0.0f, 0.0f};
    scrollDelta = 0.0f;
    keysPressed.clear();
    keysReleased.clear();
    buttonsPressed.clear();
    buttonsReleased.clear();
    doubleClicked.clear();
    // keysDown and buttonsDown persist — they represent held state
}

void Input::mouseMoveCallback(GLFWwindow *w, const double x, const double y) {
    auto *self = static_cast<Input *>(glfwGetWindowUserPointer(w));
    const glm::vec2 current = {static_cast<float>(x), static_cast<float>(y)};
    self->mouseDelta += current - self->lastPosition;
    self->lastPosition = current;
    self->mousePosition = current;
}

void Input::mouseButtonCallback(GLFWwindow *w, const int button,
                                const int action, int /*mods*/) {
    auto *self = static_cast<Input *>(glfwGetWindowUserPointer(w));

    if (action == GLFW_PRESS) {
        self->buttonsDown.insert(button);
        self->buttonsPressed.insert(button);

        // Double click detection
        const double now = glfwGetTime();
        auto it = self->lastClickTime.find(button);
        if (it != self->lastClickTime.end() &&
            (now - it->second) <= doubleClickThreshold) {
            self->doubleClicked.insert(button);
            it->second = 0.0; // reset so triple click doesn't trigger again
        } else {
            self->lastClickTime[button] = now;
        }
    } else if (action == GLFW_RELEASE) {
        self->buttonsDown.erase(button);
        self->buttonsReleased.insert(button);
    }
}

void Input::scrollCallback(GLFWwindow *w, double /*xOffset*/,
                           const double yOffset) {
    auto *self = static_cast<Input *>(glfwGetWindowUserPointer(w));
    self->scrollDelta += static_cast<float>(yOffset);
}

void Input::keyCallback(GLFWwindow *w, const int key, int /*scancode*/,
                        const int action, int /*mods*/) {
    auto *self = static_cast<Input *>(glfwGetWindowUserPointer(w));

    if (action == GLFW_PRESS) {
        self->keysDown.insert(key);
        self->keysPressed.insert(key);
    } else if (action == GLFW_RELEASE) {
        self->keysDown.erase(key);
        self->keysReleased.insert(key);
    }
    // GLFW_REPEAT intentionally ignored — use isKeyDown for held keys
}
