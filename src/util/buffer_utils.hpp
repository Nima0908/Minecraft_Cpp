#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace mc {

void appendVarInt(std::vector<uint8_t>& buffer, int32_t value);
void appendUInt16(std::vector<uint8_t>& buffer, uint16_t value);
void appendString(std::vector<uint8_t>& buffer, const std::string& str);
void appendByteArray(std::vector<uint8_t>& buffer, const std::vector<uint8_t>& bytes);
}
