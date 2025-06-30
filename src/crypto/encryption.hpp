#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mc::encryption {

std::vector<uint8_t> generateSharedSecret(size_t length = 16);

std::vector<uint8_t> rsaEncrypt(const std::vector<uint8_t> &data,
                                const std::vector<uint8_t> &publicKey);

std::string computeServerHash(const std::string &serverID,
                              const std::vector<uint8_t> &sharedSecret,
                              const std::vector<uint8_t> &publicKey);

} // namespace mc::encryption
