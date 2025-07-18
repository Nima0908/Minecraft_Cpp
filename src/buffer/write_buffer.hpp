#pragma once
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

namespace mc::buffer {

class WriteBuffer {
private:
  std::vector<ByteArray> segments_;
  size_t totalSize_ = 0;

public:
  WriteBuffer();

  template <typename T> void write(const T &value);

  template <typename T> void write(const std::vector<T> &values);

  void writeBytes(const ByteArray &data);
  void writeRaw(const void *data, size_t size);
  ByteArray compile() const;
  void clear();

  void writeBool(bool value);
  void writeInt8(int8_t value);
  void writeInt16(int16_t value);
  void writeInt32(int32_t value);
  void writeUInt16(uint16_t value);
  void writeUInt32(uint32_t value);
  void writeLong(int64_t value);
  void writeFloat(float value);
  void writeDouble(double value);
  void writeVarInt(int32_t value);
  void writeString(const std::string &str);
  void writeByteArray(const ByteArray &bytes);
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
