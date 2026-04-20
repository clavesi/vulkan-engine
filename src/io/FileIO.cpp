#include "FileIO.h"

#include <fstream>
#include <stdexcept>

namespace io {
    std::vector<char> readBinaryFile(const std::string &filename) {
        // Read the file as a binary file and start reading from end
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file " + filename);
        }

        // Starting from end means we can use the read position to determine the size of the file and allocate a buffer.
        std::vector<char> buffer(file.tellg());
        // Seek back to the beginning and read all bytes at once
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        return buffer;
    }
} // namespace io
