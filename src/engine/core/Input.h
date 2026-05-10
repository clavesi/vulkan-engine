#pragma once

#include "GLFWCallbackData.h"

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include <unordered_map>
#include <unordered_set>

class Input {
public:
    void init(GLFWwindow *window);


    // Called once per frame after all queries — clears per-frame state
    void reset();
    glm::vec2 getMouseDelta() const { return mouseDelta; }
    glm::vec2 getMousePosition() const { return mousePosition; }
    float getScrollDelta() const { return scrollDelta; }

    bool isKeyDown(const int key) const { return keysDown.count(key); }
    bool wasKeyPressed(const int key) const { return keysPressed.count(key); }
    bool wasKeyReleased(const int key) const { return keysReleased.count(key); }

    bool isMouseButtonDown(const int button) const { return buttonsDown.count(button); }
    bool wasMouseButtonPressed(const int button) const { return buttonsPressed.count(button); }
    bool wasMouseButtonReleased(const int button) const { return buttonsReleased.count(button); }
    bool wasDoubleClicked(const int button) const { return doubleClicked.count(button); }

private:
    // Mouse
    glm::vec2 mousePosition = {0.0f, 0.0f};
    glm::vec2 lastPosition = {0.0f, 0.0f};
    glm::vec2 mouseDelta = {0.0f, 0.0f};
    float scrollDelta = 0.0f;

    std::unordered_set<int> keysDown;
    std::unordered_set<int> keysPressed;
    std::unordered_set<int> keysReleased;

    std::unordered_set<int> buttonsDown;
    std::unordered_set<int> buttonsPressed;
    std::unordered_set<int> buttonsReleased;

    std::unordered_set<int> doubleClicked;
    std::unordered_map<int, double> lastClickTime;
    static constexpr double doubleClickThreshold = 0.3; // seconds

    // GLFW callbacks
    static void mouseMoveCallback(GLFWwindow *, double x, double y);
    static void mouseButtonCallback(GLFWwindow *, int button, int action, int mods);
    static void scrollCallback(GLFWwindow *, double x, double y);
    static void keyCallback(GLFWwindow *, int key, int scancode, int action, int mods);
};
