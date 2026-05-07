#pragma once

class Window;
class Input;

struct GLFWCallbackData {
    Window* window = nullptr;
    Input*  input  = nullptr;
};