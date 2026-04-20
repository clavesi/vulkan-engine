#include <cstdlib>
#include <exception>
#include <iostream>

#include "core/Engine.h"

int main() {
    try {
        EngineConfig config;
        // Customize here if desired, e.g.:
        // config.windowTitle = "My Vulkan Engine";
        // config.windowWidth = 1280;
        // config.windowHeight = 720;
        // config.shaderPath = "shaders/shader.spv"

        Engine app(config);
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
