#pragma once

#include "core/Vertex.h"

#include <string>
#include <utility>
#include <vector>

namespace io {
    // Loads an OBJ file from disk and returns deduplicated vertex/index data.
    // V coordinates are flipped to match Vulkan's top-left UV origin.
    std::pair<std::vector<Vertex>, std::vector<uint32_t> > loadObj(const std::string &path);
}
