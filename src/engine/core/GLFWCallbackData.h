#pragma once

class Window;
class Input;

struct GLFWCallbackData {
    Input*  input  = nullptr;
    Window* window = nullptr;
};