#pragma once

#include <string>
#include <cstdint>
#include <array>
#include <stdexcept>
#include "../util/logger.hpp"

namespace mc {

    static std::array<uint8_t, 16> parseDashlessUUID(const std::string& hexUUID) {
        mc::log(mc::LogLevel::DEBUG, "Parsing dashless UUID: " + hexUUID);

        if (hexUUID.size() != 32) {
            mc::log(mc::LogLevel::ERROR, "UUID length invalid: " + std::to_string(hexUUID.size()));
            throw std::invalid_argument("UUID must be exactly 32 hexadecimal characters.");
        }

        for (char c : hexUUID) {
            if (!isxdigit(c)) {
                mc::log(mc::LogLevel::ERROR, "Invalid character in UUID: " + std::string(1, c));
                throw std::invalid_argument("UUID contains non-hexadecimal characters.");
            }
        }

        std::array<uint8_t, 16> uuidBytes;
        try {
            for (size_t i = 0; i < 16; ++i) {
                std::string byteStr = hexUUID.substr(i * 2, 2);
                uuidBytes[i] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            }
        } catch (const std::exception& e) {
            mc::log(mc::LogLevel::ERROR, "Failed to parse UUID bytes: " + std::string(e.what()));
            throw;
        }

        mc::log(mc::LogLevel::DEBUG, "UUID parsed successfully");
        return uuidBytes;
    }
}
