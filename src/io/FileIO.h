#pragma once

#include <string>
#include <vector>

namespace io {
    // Reads a binary file into a byte buffer. Throws std::runtime_error on failure.
    std::vector<char> readBinaryFile(const std::string &filename);
} // namespace io
