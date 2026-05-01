#include "Mesh.h"
#include "vk/Device.h"

Mesh::Mesh(const Device &device,
           const std::vector<Vertex> &vertices,
           const std::vector<uint32_t> &indices)
    : device(device),
      // Device-local + transfer-dst because we'll stage into them
      vb(device,
         sizeof(vertices[0]) * vertices.size(),
         vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
         vk::MemoryPropertyFlagBits::eDeviceLocal),
      ib(device,
         sizeof(indices[0]) * indices.size(),
         vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
         vk::MemoryPropertyFlagBits::eDeviceLocal),
      idxCount(static_cast<uint32_t>(indices.size())) {
    vb.uploadViaStaging(vertices.data(), sizeof(vertices[0]) * vertices.size());
    ib.uploadViaStaging(indices.data(), sizeof(indices[0]) * indices.size());
}
