#pragma once

#include <vector>
#include <cstdint>

namespace mc::compression {


std::vector<uint8_t> compress(const std::vector<uint8_t>& input);


std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);

} 
