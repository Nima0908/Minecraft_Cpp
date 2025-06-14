#pragma once

#include <vector>
#include <cstdint>

namespace mc::encryption {

std::vector<uint8_t> generateSharedSecret(size_t length = 16);

std::vector<uint8_t> rsaEncrypt(const std::vector<uint8_t>& data, const std::vector<uint8_t>& publicKey);

}
