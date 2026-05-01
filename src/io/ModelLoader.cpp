#include "ModelLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stdexcept>
#include <unordered_map>

namespace io {
    std::pair<std::vector<Vertex>, std::vector<uint32_t> > loadObj(const std::string &path) {
        tinyobj::attrib_t attrib; // holds vertices, normals, and texcoords
        std::vector<tinyobj::shape_t> shapes; // holds all separate objects and their faces
        std::vector<tinyobj::material_t> materials;
        std::string warn, err; // warnings and errors while loading

        // LoadObj triangulates by default, so each face is guaranteed to be 3 vertices
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        // Tracks which Vertex values we've already emitted, so duplicates collapse to one entry
        std::unordered_map<Vertex, uint32_t> uniqueVertices;

        for (const auto &shape: shapes) {
            for (const auto &index: shape.mesh.indices) {
                Vertex vertex{};

                // attrib.vertices is a flat float array; 3 floats per position
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                // OBJ's V origin is bottom-left; Vulkan's is top-left, so flip
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                // Only emit a new vertex if we haven't seen this exact combination before
                if (auto it = uniqueVertices.find(vertex); it == uniqueVertices.end()) {
                    const auto newIndex = static_cast<uint32_t>(vertices.size());
                    uniqueVertices[vertex] = newIndex;
                    vertices.push_back(vertex);
                    indices.push_back(newIndex);
                } else {
                    indices.push_back(it->second);
                }
            }
        }

        return {std::move(vertices), std::move(indices)};
    }
}
