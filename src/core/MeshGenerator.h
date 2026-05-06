#pragma once

#include "Vertex.h"

#include <utility>
#include <vector>

namespace MeshGenerator {
    // Generate a UV sphere generated at the origin with the given radius.
    // rings are horizontal divisions. more = smoother poles
    // sectors are vertical divisions. more = smoother outline
    std::pair<std::vector<Vertex>, std::vector<uint32_t>> sphere(float radius=1.0f, uint32_t rings=32, uint32_t sectors=32);
}

