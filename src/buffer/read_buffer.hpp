#pragma once
#include "../util/performance_hints.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace mc::buffer {

class ReadBuffer {
private:
  ByteArray data_;
  size_t readPos_ = 0;

public:
  explicit ReadBuffer(ByteArray data);

  template <typename T> T read();

  MC_FORCE_INLINE bool ensure(size_t len) const;
  ByteArray readBytes(size_t len);
  uint8_t readByte();
  int8_t readInt8();
  bool readBool();
  int16_t readInt16();
  int32_t readInt32();
  uint8_t readUInt8();
  uint16_t readUInt16();
  uint32_t readUInt32();
  int32_t readVarInt();
  int64_t readLong();
  float readFloat();
  double readDouble();
  std::string readString();
  ByteArray readByteArray();
  ByteArray copyRemaining() const;
  size_t remaining() const;
  const ByteArray &data() const;
};

template <typename T> T ReadBuffer::read() {
  static_assert(std::is_trivially_copyable_v<T>,
                "Only trivial types supported");
  if (!ensure(sizeof(T)))
    throw std::runtime_error("Generic read out of bounds");
  T val;
  std::memcpy(&val, &data_[readPos_], sizeof(T));
  readPos_ += sizeof(T);
  return val;
}

} // namespace mc::buffer
