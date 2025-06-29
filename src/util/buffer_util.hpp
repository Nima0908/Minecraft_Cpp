#pragma once
#include <filesystem>
#include <string>
#include <cstdint>
#include <vector>
#include <array>
#include <stdexcept>

namespace mc {
    class BufferUtil {
    private:
        std::vector<uint8_t> buffer_;
        size_t readPos_;

    public:
        BufferUtil();
        BufferUtil(std::vector<uint8_t> data);

        void writeVarInt(int32_t value);
        void writeUInt16(uint16_t value);
        void writeString(const std::string& str);
        void writeByteArray(const std::vector<uint8_t>& bytes);
        void writeBytes(const std::vector<uint8_t>& bytes);

        int32_t readVarInt();
        uint16_t readUInt16();
        uint8_t readByte(); 
        std::string readString();
        std::vector<uint8_t> readByteArray();
        std::vector<uint8_t> readBytes(size_t length);

        // Utility methods
        size_t remaining() const;
        const std::vector<uint8_t>& data() const;
    };

    // UUID utility functions
    std::array<uint8_t, 16> parseDashlessUUID(const std::string& hexUUID);
    std::string formatUUIDFromBytes(const std::vector<uint8_t>& uuidBytes);
}
