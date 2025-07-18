#pragma once

#include <stdint.h>

namespace mc::datatypes::nbt {

enum class NBTTagType : uint8_t {
  End = 0,
  Byte = 1,
  Short = 2,
  Int = 3,
  Long = 4,
  Float = 5,
  Double = 6,
  ByteArray = 7,
  String = 8,
  List = 9,
  Compound = 10,
  IntArray = 11,
  LongArray = 12
};
} // namespace mc::datatypes::nbt
