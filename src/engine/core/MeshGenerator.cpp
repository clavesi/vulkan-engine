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
        return {vertices, {}};
    }

    std::pair<std::vector<Vertex>, std::vector<uint32_t> > disc(
        float innerRadius, float outerRadius, uint32_t segments
    ) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        for (uint32_t i = 0; i <= segments; ++i) {
            const float angle = (static_cast<float>(i) / static_cast<float>(segments)) * glm::two_pi<float>();
            const float cosA = std::cos(angle);
            const float sinA = std::sin(angle);

            // Inner vertex — UV.x = 0
            Vertex inner{};
            inner.pos = {cosA * innerRadius, sinA * innerRadius, 0.0f};
            inner.normal = {0.0f, 0.0f, 1.0f};
            inner.texCoord = {0.0f, static_cast<float>(i) / static_cast<float>(segments)};
            inner.color = {1.0f, 1.0f, 1.0f};
            vertices.push_back(inner);

            // Outer vertex — UV.x = 1
            Vertex outer{};
            outer.pos = {cosA * outerRadius, sinA * outerRadius, 0.0f};
            outer.normal = {0.0f, 0.0f, 1.0f};
            outer.texCoord = {1.0f, static_cast<float>(i) / static_cast<float>(segments)};
            outer.color = {1.0f, 1.0f, 1.0f};
            vertices.push_back(outer);
        }

        // Two triangles per segment
        for (uint32_t i = 0; i < segments; ++i) {
            const uint32_t base = i * 2;
            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 1);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        }

        return {std::move(vertices), std::move(indices)};
    }

    std::pair<std::vector<Vertex>, std::vector<uint32_t> > cube() {
        // 24 vertices (4 per face) so each face has correct winding
        const std::vector<glm::vec3> positions = {
            // +X
            {1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {1, -1, 1},
            // -X
            {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}, {-1, -1, -1},
            // +Y
            {-1, 1, -1}, {-1, 1, 1}, {1, 1, 1}, {1, 1, -1},
            // -Y
            {-1, -1, 1}, {-1, -1, -1}, {1, -1, -1}, {1, -1, 1},
            // +Z
            {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1},
            // -Z
            {1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1},
        };

        std::vector<Vertex> vertices;
        for (const auto &p: positions) {
            Vertex v{};
            v.pos = p;
            vertices.push_back(v);
        }

        std::vector<uint32_t> indices;
        for (uint32_t face = 0; face < 6; ++face) {
            const uint32_t base = face * 4;
            indices.insert(indices.end(), {
                               base + 0, base + 1, base + 2,
                               base + 2, base + 3, base + 0
                           });
        }

        return {std::move(vertices), std::move(indices)};
    }
}
