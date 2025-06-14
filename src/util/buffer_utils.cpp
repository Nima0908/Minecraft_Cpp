#include "buffer_utils.hpp"

namespace mc {

void appendVarInt(std::vector<uint8_t>& buffer, int32_t value) {
    do {
        uint8_t temp = value & 0x7F;
        value >>= 7;
        if (value != 0) {
            temp |= 0x80;
        }
        buffer.push_back(temp);
    } while (value != 0);
}

void appendUInt16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);
}

void appendString(std::vector<uint8_t>& buffer, const std::string& str) {
    appendVarInt(buffer, str.size());
    buffer.insert(buffer.end(), str.begin(), str.end());
}
void appendByteArray(std::vector<uint8_t>& buffer, const std::vector<uint8_t>& bytes) {
    appendVarInt(buffer, static_cast<int>(bytes.size()));
    buffer.insert(buffer.end(), bytes.begin(), bytes.end());
}

}
