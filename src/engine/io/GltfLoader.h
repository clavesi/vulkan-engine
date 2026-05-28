#pragma once

#include "core/Mesh.h"
#include "vk/Texture.h"

#include <string>
#include <vector>

class Device;

namespace io {
    struct LoadedMesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    // Loads the first primitive of the first mesh in a glTF file.
    // Returns vertex/index data ready to upload to a Mesh.
    // Textures are loaded separately via loadGltfBaseColorTexture.
    LoadedMesh loadGltfMesh(const std::string &path);

    // Loads the base color texture from the first material in a glTF file.
    // Returns false if no texture is found (caller can use a default).
    bool loadGltfBaseColorTexture(const Device &device, const std::string &path, Texture &out);
}
