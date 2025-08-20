#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mc::buffer {
using ByteArray = std::vector<uint8_t>;

// Pre-allocate common buffer sizes for better performance
constexpr size_t SMALL_BUFFER_SIZE = 256;
constexpr size_t MEDIUM_BUFFER_SIZE = 1024;
constexpr size_t LARGE_BUFFER_SIZE = 4096;

class ReadBuffer;
class WriteBuffer;
} // namespace mc::buffer
