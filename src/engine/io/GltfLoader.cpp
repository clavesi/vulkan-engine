#include "GltfLoader.h"
#include "vk/Device.h"
#include "vk/TextureLoader.h"
#include "core/Vertex.h"

#include <tiny_gltf.h>
#include <stdexcept>

namespace io {
    static tinygltf::Model loadModel(const std::string &path) {
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string err, warn;

        // We handle image loading ourselves so give tinygltf a no-op callback
        loader.SetImageLoader([](tinygltf::Image *, const int, std::string *, std::string *,
                                 int, int, const unsigned char *, int, void *) {
            return true;
        }, nullptr);

        const bool isBinary = path.ends_with(".glb");
        const bool ok = isBinary
                            ? loader.LoadBinaryFromFile(&model, &err, &warn, path)
                            : loader.LoadASCIIFromFile(&model, &err, &warn, path);

        if (!warn.empty()) fprintf(stderr, "glTF warning: %s\n", warn.c_str());
        if (!ok) throw std::runtime_error("failed to load glTF: " + path + "\n" + err);

        return model;
    }

    LoadedMesh loadGltfMesh(const std::string &path) {
        const auto model = loadModel(path);

        if (model.meshes.empty()) throw std::runtime_error("glTF has no meshes: " + path);
        const auto &mesh = model.meshes[0];
        if (mesh.primitives.empty()) throw std::runtime_error("glTF mesh has no primitives: " + path);
        const auto &prim = mesh.primitives[0];

        LoadedMesh result;

        // Helper to get accessor data as a typed span
        auto getBuffer = [&](const std::string &attr) -> const uint8_t * {
            const auto it = prim.attributes.find(attr);
            if (it == prim.attributes.end()) return nullptr;
            const auto &acc = model.accessors[it->second];
            const auto &view = model.bufferViews[acc.bufferView];
            return model.buffers[view.buffer].data.data() + view.byteOffset + acc.byteOffset;
        };

        auto getCount = [&](const std::string &attr) -> size_t {
            const auto it = prim.attributes.find(attr);
            if (it == prim.attributes.end()) return 0;
            return model.accessors[it->second].count;
        };

        const uint8_t *posData = getBuffer("POSITION");
        const uint8_t *normData = getBuffer("NORMAL");
        const uint8_t *uvData = getBuffer("TEXCOORD_0");
        const size_t vertCount = getCount("POSITION");

        if (!posData) throw std::runtime_error("glTF mesh has no POSITION attribute");

        // Get byte strides
        auto getStride = [&](const std::string &attr, const size_t defaultStride) -> size_t {
            const auto it = prim.attributes.find(attr);
            if (it == prim.attributes.end()) return defaultStride;
            const auto &view = model.bufferViews[model.accessors[it->second].bufferView];
            return view.byteStride > 0 ? view.byteStride : defaultStride;
        };

        const size_t posStride = getStride("POSITION", sizeof(float) * 3);
        const size_t normStride = getStride("NORMAL", sizeof(float) * 3);
        const size_t uvStride = getStride("TEXCOORD_0", sizeof(float) * 2);

        result.vertices.resize(vertCount);
        for (size_t i = 0; i < vertCount; ++i) {
            Vertex v{};

            auto pos = reinterpret_cast<const float *>(posData + i * posStride);
            v.pos = {pos[0], pos[1], pos[2]};

            if (normData) {
                auto norm = reinterpret_cast<const float *>(normData + i * normStride);
                v.normal = {norm[0], norm[1], norm[2]};
            }

            if (uvData) {
                auto uv = reinterpret_cast<const float *>(uvData + i * uvStride);
                v.texCoord = {uv[0], uv[1]};
            }

            v.color = {1.0f, 1.0f, 1.0f};
            result.vertices[i] = v;
        }

        // Indices
        if (prim.indices >= 0) {
            const auto &acc = model.accessors[prim.indices];
            const auto &view = model.bufferViews[acc.bufferView];
            const uint8_t *data = model.buffers[view.buffer].data.data()
                                  + view.byteOffset + acc.byteOffset;

            result.indices.resize(acc.count);
            for (size_t i = 0; i < acc.count; ++i) {
                switch (acc.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        result.indices[i] = reinterpret_cast<const uint16_t *>(data)[i];
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        result.indices[i] = reinterpret_cast<const uint32_t *>(data)[i];
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        result.indices[i] = data[i];
                        break;
                    default:
                        throw std::runtime_error("unsupported index component type");
                }
            }
        }

        return result;
    }

    bool loadGltfBaseColorTexture(const Device &device, const std::string &path, Texture &out) {
        if (!path.ends_with(".glb")) return false;

        const auto model = loadModel(path);

        if (model.materials.empty()) return false;
        const auto &mat = model.materials[0];

        const int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
        if (texIndex < 0) return false;

        const auto &tex = model.textures[texIndex];
        const auto &img = model.images[tex.source];

        if (img.bufferView < 0) return false;

        const auto &view = model.bufferViews[img.bufferView];
        const uint8_t *data = model.buffers[view.buffer].data.data() + view.byteOffset;
        vk_util::loadTextureFromMemory(device, data, view.byteLength, out);
        return true;
    }
}
