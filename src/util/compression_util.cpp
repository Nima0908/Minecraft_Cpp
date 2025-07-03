#include "compression_util.hpp"
#include "logger.hpp"
#include <sstream>
#include <stdexcept>
#include <zlib.h>

#include <iomanip>

namespace mc::utils {

std::vector<uint8_t> compress(const std::vector<uint8_t> &input) {

  if (input.empty()) {
    return {};
  }

  uLongf compressedSize = compressBound(input.size());

  std::vector<uint8_t> output(compressedSize);
  int res = ::compress2(output.data(), &compressedSize, input.data(),
                        input.size(), Z_BEST_SPEED);

  if (res != Z_OK) {
    std::ostringstream oss;
    oss << "Failed to compress data, zlib error code: " << res;
    mc::utils::log(mc::utils::LogLevel::ERROR, oss.str());
    throw std::runtime_error("Failed to compress data");
  }

  output.resize(compressedSize);

  double compressionRatio = static_cast<double>(input.size()) / compressedSize;
  std::ostringstream oss;
  oss << "Compression successful: " << input.size() << " -> " << compressedSize
      << " bytes (ratio: " << std::fixed << std::setprecision(2)
      << compressionRatio << ")";
  mc::utils::log(mc::utils::LogLevel::INFO, oss.str());

  return output;
}

std::vector<uint8_t> decompress(const std::vector<uint8_t> &input) {

  if (input.empty()) {
    return {};
  }

  uLongf decompressedSize = input.size() * 4;

  std::vector<uint8_t> output(decompressedSize);
  int res = ::uncompress(output.data(), &decompressedSize, input.data(),
                         input.size());

  if (res == Z_BUF_ERROR) {
    mc::utils::log(mc::utils::LogLevel::WARN,
                   "Initial buffer too small, doubling size and retrying");
    decompressedSize *= 2;
    output.resize(decompressedSize);
    res = ::uncompress(output.data(), &decompressedSize, input.data(),
                       input.size());
    mc::utils::log(mc::utils::LogLevel::DEBUG,
                   "Retry with buffer size: " +
                       std::to_string(decompressedSize) + " bytes");
  }

  if (res != Z_OK) {
    std::ostringstream oss;
    oss << "Failed to decompress data, zlib error code: " << res;
    mc::utils::log(mc::utils::LogLevel::ERROR, oss.str());
    throw std::runtime_error("Failed to decompress data");
  }

  output.resize(decompressedSize);
  mc::utils::log(mc::utils::LogLevel::INFO,
                 "Decompression successful: " + std::to_string(input.size()) +
                     " -> " + std::to_string(decompressedSize) + " bytes");

  return output;
}
} // namespace mc::utils
