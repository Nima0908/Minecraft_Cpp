#pragma once
#include "../nbt_tag.hpp"
#include "nbt_byte.hpp"
#include "nbt_byte_array.hpp"
#include "nbt_compound.hpp"
#include "nbt_double.hpp"
#include "nbt_end.hpp"
#include "nbt_float.hpp"
#include "nbt_int.hpp"
#include "nbt_int_array.hpp"
#include "nbt_list.hpp"
#include "nbt_long.hpp"
#include "nbt_long_array.hpp"
#include "nbt_short.hpp"
#include "nbt_string.hpp"

namespace mc::datatypes::nbt::tags {

// Factory function to create NBT tags
inline std::unique_ptr<NBTTag> createTag(NBTTagType type) {
  switch (type) {
  case NBTTagType::End:
    return std::make_unique<tags::NBTEnd>();
  case NBTTagType::Byte:
    return std::make_unique<tags::NBTByte>();
  case NBTTagType::Short:
    return std::make_unique<tags::NBTShort>();
  case NBTTagType::Int:
    return std::make_unique<tags::NBTInt>();
  case NBTTagType::Long:
    return std::make_unique<tags::NBTLong>();
  case NBTTagType::Float:
    return std::make_unique<tags::NBTFloat>();
  case NBTTagType::Double:
    return std::make_unique<tags::NBTDouble>();
  case NBTTagType::ByteArray:
    return std::make_unique<tags::NBTByteArray>();
  case NBTTagType::String:
    return std::make_unique<tags::NBTString>();
  case NBTTagType::List:
    return std::make_unique<tags::NBTList>();
  case NBTTagType::Compound:
    return std::make_unique<tags::NBTCompound>();
  case NBTTagType::IntArray:
    return std::make_unique<tags::NBTIntArray>();
  case NBTTagType::LongArray:
    return std::make_unique<tags::NBTLongArray>();
  default:
    return nullptr;
  }
}

} // namespace mc::datatypes::nbt::tags
