#include "MeshGenerator.h"

#include <glm/gtc/constants.hpp>
#include <cmath>

namespace MeshGenerator {
    std::pair<std::vector<Vertex>, std::vector<uint32_t> > sphere(
        const float radius,
        const uint32_t rings,
        const uint32_t sectors
    ) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        vertices.reserve((rings + 1) * (sectors + 1));

        const float ringStep = glm::pi<float>() / rings;
        const float sectorStep = 2.0f * glm::pi<float>() / sectors;

        for (uint32_t r = 0; r <= rings; r++) {
            const float phi = r * ringStep; // +pi/2 to -pi/2
            const float cosPhi = std::cos(phi);
            const float sinPhi = std::sin(phi);

            for (uint32_t s = 0; s <= sectors; s++) {
                const float theta = s * sectorStep;
                const float cosTheta = std::cos(theta);
                const float sinTheta = std::sin(theta);

                // Position on unit sphere, scaled by radius
                const glm::vec3 normal = {
                    sinPhi * cosTheta,
                    sinPhi * sinTheta,
                    cosPhi
                };

                Vertex v{};
                v.pos = normal * radius;
                v.normal = normal; // unit sphere so normal = normalized position
                v.color = {1.0f, 1.0f, 1.0f};
                v.texCoord = {
                    static_cast<float>(s) / sectors, // U: 0..1 around equator
                    static_cast<float>(r) / rings // V: 0..1 top to bottom
                };

                vertices.push_back(v);
            }
        }

        // Build indices - two triangles per quad between adjacent rings/sectors
        indices.reserve(rings * sectors * 6);

        for (uint32_t r = 0; r < rings; ++r) {
            for (uint32_t s = 0; s < sectors; ++s) {
                const uint32_t curr = r * (sectors + 1) + s;
                const uint32_t next = curr + (sectors + 1);

                // First triangle
                indices.push_back(curr);
                indices.push_back(next);
                indices.push_back(curr + 1);

                // Second triangle
                indices.push_back(curr + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }

        return {std::move(vertices), std::move(indices)};
    }

    std::pair<std::vector<Vertex>, std::vector<uint32_t> > circle(
        const float radius,
        const uint32_t segments
    ) {
        std::vector<Vertex> vertices;
        vertices.reserve(segments + 1);

        for (uint32_t i = 0; i <= segments; ++i) {
            const float angle = (static_cast<float>(i) / static_cast<float>(segments)) * glm::two_pi<float>();
            Vertex v{};
            v.pos = {std::cos(angle) * radius, std::sin(angle) * radius, 0.0f};
            vertices.push_back(v);
        }

        // Empty indices — drawn as line strip directly
        return {std::move(vertices), {}};
    }
}
