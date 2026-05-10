#pragma once

#include "vk/Buffer.h"
#include "core/Vertex.h"

#include <cstdint>
#include <vector>

class Device;

class Mesh {
public:
    Mesh(const Device &device, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    // Movable for std::optional<Mesh>; not move-assignable because of the const Device& reference
    Mesh(Mesh &&) noexcept = default;
    Mesh &operator=(Mesh &&) = delete;

    const Buffer &vertexBuffer() const { return vb; }
    const Buffer &indexBuffer() const { return ib; }
    uint32_t indexCount() const { return idxCount; }

private:
    const Device &device;
    Buffer vb;
    Buffer ib;
    uint32_t idxCount;
};
