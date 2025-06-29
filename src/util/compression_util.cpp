#include "compression_util.hpp"
#include <zlib.h>
#include <stdexcept>

namespace mc::compression {

std::vector<uint8_t> compress(const std::vector<uint8_t>& input) {
    if (input.empty()) return {};

    uLongf compressedSize = compressBound(input.size());
    std::vector<uint8_t> output(compressedSize);

    int res = ::compress2(output.data(), &compressedSize, input.data(), input.size(), Z_BEST_SPEED);
    if (res != Z_OK) {
        throw std::runtime_error("Failed to compress data");
    }

    output.resize(compressedSize);
    return output;
}

std::vector<uint8_t> decompress(const std::vector<uint8_t>& input) {
    if (input.empty()) return {};

    uLongf decompressedSize = input.size() * 4;
    std::vector<uint8_t> output(decompressedSize);

    int res = ::uncompress(output.data(), &decompressedSize, input.data(), input.size());
    if (res == Z_BUF_ERROR) {
        decompressedSize *= 2;
        output.resize(decompressedSize);
        res = ::uncompress(output.data(), &decompressedSize, input.data(), input.size());
    }

    if (res != Z_OK) {
        throw std::runtime_error("Failed to decompress data");
    }

    output.resize(decompressedSize);
    return output;
}

}
