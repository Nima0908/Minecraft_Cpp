#pragma once

#include "../util/logger.hpp"
#include <array>
#include <cstdint>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

namespace mc::utils {

static std::array<uint8_t, 16> parseDashlessUUID(const std::string &hexUUID) {
  mc::utils::log(mc::utils::LogLevel::DEBUG,
                 "Parsing dashless UUID: " + hexUUID);
  if (hexUUID.size() != 32) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "UUID length invalid: " + std::to_string(hexUUID.size()));
    throw std::invalid_argument(
        "UUID must be exactly 32 hexadecimal characters.");
  }
  for (char c : hexUUID) {
    if (!isxdigit(c)) {
      mc::utils::log(mc::utils::LogLevel::ERROR,
                     "Invalid character in UUID: " + std::string(1, c));
      throw std::invalid_argument("UUID contains non-hexadecimal characters.");
    }
  }
  std::array<uint8_t, 16> uuidBytes;
  try {
    for (size_t i = 0; i < 16; ++i) {
      std::string byteStr = hexUUID.substr(i * 2, 2);
      uuidBytes[i] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
    }
  } catch (const std::exception &e) {
    mc::utils::log(mc::utils::LogLevel::ERROR,
                   "Failed to parse UUID bytes: " + std::string(e.what()));
    throw;
  }
  return uuidBytes;
}

static std::string formatUUIDFromBytes(const std::vector<uint8_t> &uuidBytes) {
  if (uuidBytes.size() != 16) {
    throw std::invalid_argument("UUID must be exactly 16 bytes");
  }

  std::stringstream ss;
  ss << std::hex << std::setfill('0');

  for (size_t i = 0; i < 16; ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      ss << '-';
    }
    ss << std::setw(2) << static_cast<unsigned>(uuidBytes[i]);
  }

  return ss.str();
}

} // namespace mc::utils
