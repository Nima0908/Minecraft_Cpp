#include "write_buffer.hpp"
#include <cstring>

namespace mc::buffer {

WriteBuffer::WriteBuffer(size_t initial_capacity)
    : capacity_(initial_capacity) {
  buffer_.reserve(capacity_);
}

void WriteBuffer::writeBytes(const ByteArray &data) {
  buffer_.insert(buffer_.end(), data.begin(), data.end());
}

void WriteBuffer::writeRaw(const void *data, size_t size) {
  const uint8_t *bytes = static_cast<const uint8_t *>(data);
  buffer_.insert(buffer_.end(), bytes, bytes + size);
}

void WriteBuffer::clear() {
  buffer_.clear();
  buffer_.reserve(capacity_);
}

void WriteBuffer::reserve(size_t size) {
  if (size > capacity_) {
    capacity_ = size;
    buffer_.reserve(capacity_);
  }
}

void WriteBuffer::writeBool(bool value) {
  buffer_.push_back(static_cast<uint8_t>(value ? 1 : 0));
}

void WriteBuffer::writeInt8(int8_t value) { writeRaw(&value, sizeof(value)); }

void WriteBuffer::writeInt16(int16_t value) {
  buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
}

void WriteBuffer::writeInt32(int32_t value) {
  buffer_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
  buffer_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
  buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
}

void WriteBuffer::writeUInt8(uint8_t value) { writeRaw(&value, sizeof(value)); }

void WriteBuffer::writeUInt16(uint16_t value) {
  buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
}

void WriteBuffer::writeUInt32(uint32_t value) {
  buffer_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
  buffer_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
  buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
}

void WriteBuffer::writeLong(int64_t value) {
  for (int i = 7; i >= 0; --i) {
    buffer_.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
  }
}

void WriteBuffer::writeFloat(float value) {
  uint32_t raw;
  std::memcpy(&raw, &value, sizeof(float));
  writeUInt32(raw);
}

void WriteBuffer::writeDouble(double value) {
  uint64_t raw;
  std::memcpy(&raw, &value, sizeof(double));
  for (int i = 7; i >= 0; --i) {
    buffer_.push_back(static_cast<uint8_t>((raw >> (i * 8)) & 0xFF));
  }
}

void WriteBuffer::writeVarInt(int32_t value) {
  do {
    uint8_t temp = value & 0x7F;
    value >>= 7;
    if (value != 0)
      temp |= 0x80;
    buffer_.push_back(temp);
  } while (value != 0);
}

void WriteBuffer::writeString(const std::string &str) {
  writeVarInt(static_cast<int32_t>(str.size()));
  buffer_.insert(buffer_.end(), str.begin(), str.end());
}

void WriteBuffer::writeString(std::string_view str) {
  writeVarInt(static_cast<int32_t>(str.size()));
  buffer_.insert(buffer_.end(), str.begin(), str.end());
}

void WriteBuffer::writeByteArray(const ByteArray &bytes) {
  writeVarInt(static_cast<int32_t>(bytes.size()));
  writeBytes(bytes);
}

void WriteBuffer::writeBytes(ByteArray &&data) {
  if (buffer_.empty()) {
    buffer_ = std::move(data);
  } else {
    buffer_.insert(buffer_.end(), std::make_move_iterator(data.begin()),
                   std::make_move_iterator(data.end()));
  }
}

} // namespace mc::buffer
