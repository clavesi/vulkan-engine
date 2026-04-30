#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    // Describes at which rate to load data from memory throughout the vertices
    static vk::VertexInputBindingDescription getBindingDescription() {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    // Describes how to extract a vertex attribute from a chunk of vertex data
    // from a binding description
    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        return {
            {
                {.location = 0,.binding = 0,.format = vk::Format::eR32G32B32Sfloat,.offset = offsetof(Vertex, pos)},
                {.location = 1,.binding = 0,.format = vk::Format::eR32G32B32Sfloat,.offset = offsetof(Vertex, color)},
                {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, texCoord)}
            }
        };
    }
};
