#pragma once

#include "UniformBufferObject.h"

#include <glm/gtx/hash.hpp>
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
                {.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, pos)},
                {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)},
                {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, texCoord)}
            }
        };
    }

    bool operator==(const Vertex &other) const {
        return pos == other.pos
               && color == other.color
               && texCoord == other.texCoord;
    }
};

// std::hash specialization so Vertex can be used as an unordered_map key.
// Standard combine pattern from cppreference; collisions are fine, equality is what matters for correctness.
namespace std {
    template<>
    struct hash<Vertex> {
        size_t operator()(const Vertex &v) const noexcept {
            return ((hash<glm::vec3>()(v.pos)
                     ^ (hash<glm::vec3>()(v.color) << 1)) >> 1)
                   ^ (hash<glm::vec2>()(v.texCoord) << 1);
        }
    };
}
