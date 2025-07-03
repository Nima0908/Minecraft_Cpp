#pragma once

#include <cstdint>
#include <vector>

namespace mc::utils {

std::vector<uint8_t> compress(const std::vector<uint8_t> &input);

std::vector<uint8_t> decompress(const std::vector<uint8_t> &input);

} // namespace mc::utils
