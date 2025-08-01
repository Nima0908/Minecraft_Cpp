#include "write_buffer.hpp"
#include <cstring>

namespace mc::buffer {

WriteBuffer::WriteBuffer() = default;

void WriteBuffer::writeBytes(const ByteArray &data) {
  segments_.push_back(data);
  totalSize_ += data.size();
}

void WriteBuffer::writeRaw(const void *data, size_t size) {
  ByteArray buf((const uint8_t *)data, (const uint8_t *)data + size);
  writeBytes(buf);
}

ByteArray WriteBuffer::compile() const {
  ByteArray out;
  out.reserve(totalSize_);
  for (const auto &segment : segments_)
    out.insert(out.end(), segment.begin(), segment.end());
  return out;
}

void WriteBuffer::clear() {
  segments_.clear();
  totalSize_ = 0;
}

void WriteBuffer::writeBool(bool value) {
  writeBytes({static_cast<uint8_t>(value ? 1 : 0)});
}

void WriteBuffer::writeInt8(int8_t value) { writeRaw(&value, sizeof(value)); }

void WriteBuffer::writeInt16(int16_t value) {
  ByteArray buf = {static_cast<uint8_t>((value >> 8) & 0xFF),
                   static_cast<uint8_t>(value & 0xFF)};
  writeBytes(buf);
}

void WriteBuffer::writeInt32(int32_t value) {
  ByteArray buf = {static_cast<uint8_t>((value >> 24) & 0xFF),
                   static_cast<uint8_t>((value >> 16) & 0xFF),
                   static_cast<uint8_t>((value >> 8) & 0xFF),
                   static_cast<uint8_t>(value & 0xFF)};
  writeBytes(buf);
}

void WriteBuffer::writeUInt8(uint8_t value) { writeRaw(&value, sizeof(value)); }

void WriteBuffer::writeUInt16(uint16_t value) {
  ByteArray buf = {static_cast<uint8_t>((value >> 8) & 0xFF),
                   static_cast<uint8_t>(value & 0xFF)};
  writeBytes(buf);
}

void WriteBuffer::writeUInt32(uint32_t value) {
  ByteArray buf = {static_cast<uint8_t>((value >> 24) & 0xFF),
                   static_cast<uint8_t>((value >> 16) & 0xFF),
                   static_cast<uint8_t>((value >> 8) & 0xFF),
                   static_cast<uint8_t>(value & 0xFF)};
  writeBytes(buf);
}

void WriteBuffer::writeLong(int64_t value) {
  ByteArray buf(8);
  for (int i = 7; i >= 0; --i) {
    buf[7 - i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
  }
  writeBytes(buf);
}

void WriteBuffer::writeFloat(float value) {
  uint32_t raw;
  std::memcpy(&raw, &value, sizeof(float));
  writeUInt32(raw);
}

void WriteBuffer::writeDouble(double value) {
  uint64_t raw;
  std::memcpy(&raw, &value, sizeof(double));
  ByteArray buf(8);
  for (int i = 7; i >= 0; --i) {
    buf[7 - i] = static_cast<uint8_t>((raw >> (i * 8)) & 0xFF);
  }
  writeBytes(buf);
}

void WriteBuffer::writeVarInt(int32_t value) {
  do {
    uint8_t temp = value & 0x7F;
    value >>= 7;
    if (value != 0)
      temp |= 0x80;
    writeBytes({temp});
  } while (value != 0);
}

void WriteBuffer::writeString(const std::string &str) {
  writeVarInt(static_cast<int32_t>(str.size()));
  writeBytes(ByteArray(str.begin(), str.end()));
}

void WriteBuffer::writeByteArray(const ByteArray &bytes) {
  writeVarInt(static_cast<int32_t>(bytes.size()));
  writeBytes(bytes);
}

} // namespace mc::buffer
