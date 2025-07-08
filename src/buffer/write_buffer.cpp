#include "write_buffer.hpp"
#include <cstring>

namespace mc::buffer {

WriteBuffer::WriteBuffer() = default;

void WriteBuffer::writeBytes(const ByteArray& data) {
    segments_.push_back(data);
    totalSize_ += data.size();
}

void WriteBuffer::writeRaw(const void* data, size_t size) {
    ByteArray buf((const uint8_t*)data, (const uint8_t*)data + size);
    writeBytes(buf);
}

ByteArray WriteBuffer::compile() const {
    ByteArray out;
    out.reserve(totalSize_);
    for (const auto& segment : segments_)
        out.insert(out.end(), segment.begin(), segment.end());
    return out;
}

void WriteBuffer::clear() {
    segments_.clear();
    totalSize_ = 0;
}

void WriteBuffer::writeVarInt(int32_t value) {
    do {
        uint8_t temp = value & 0x7F;
        value >>= 7;
        if (value != 0) temp |= 0x80;
        writeBytes({ temp });
    } while (value != 0);
}

void WriteBuffer::writeUInt16(uint16_t value) {
    writeBytes({ (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) });
}

void WriteBuffer::writeString(const std::string& str) {
    writeVarInt(static_cast<int32_t>(str.size()));
    writeBytes(ByteArray(str.begin(), str.end()));
}

void WriteBuffer::writeByteArray(const ByteArray& bytes) {
    writeVarInt(static_cast<int32_t>(bytes.size()));
    writeBytes(bytes);
}

} // namespace mc::buffer
