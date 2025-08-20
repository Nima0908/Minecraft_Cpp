#pragma once
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace mc::buffer {

class WriteBuffer {
private:
  ByteArray buffer_;
  size_t capacity_;

public:
  explicit WriteBuffer(size_t initial_capacity = MEDIUM_BUFFER_SIZE);

  template <typename T> void write(const T &value);

  template <typename T> void write(const std::vector<T> &values);

  void writeBytes(const ByteArray &data);
  void writeRaw(const void *data, size_t size);
  const ByteArray &data() const { return buffer_; }
  ByteArray compile() const { return buffer_; }
  void clear();
  void reserve(size_t size);
  size_t size() const { return buffer_.size(); }

  void writeBool(bool value);
  void writeInt8(int8_t value);
  void writeInt16(int16_t value);
  void writeInt32(int32_t value);
  void writeUInt8(uint8_t value);
  void writeUInt16(uint16_t value);
  void writeUInt32(uint32_t value);
  void writeLong(int64_t value);
  void writeFloat(float value);
  void writeDouble(double value);
  void writeVarInt(int32_t value);
  void writeString(const std::string &str);
  void writeString(std::string_view str);
  void writeByteArray(const ByteArray &bytes);

  // Move semantics for better performance
  void writeBytes(ByteArray &&data);

  // Bulk operations
  template <typename Iterator> void writeRange(Iterator begin, Iterator end) {
    buffer_.insert(buffer_.end(), begin, end);
  }
};

template <typename T> void WriteBuffer::write(const T &value) {
  static_assert(std::is_trivially_copyable_v<T>,
                "Only trivial types supported");
  writeRaw(&value, sizeof(T));
}

template <typename T> void WriteBuffer::write(const std::vector<T> &values) {
  for (const auto &v : values)
    write(v);
}

} // namespace mc::buffer
