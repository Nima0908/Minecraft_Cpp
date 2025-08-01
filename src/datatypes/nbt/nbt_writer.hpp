#pragma once
#include "nbt_tag.hpp"

namespace mc::datatypes::nbt {
class NBTWriter {
public:
  static void writeNamedTag(mc::buffer::WriteBuffer &out,
                            const std::string &name, const NBTTag &tag) {
    out.writeUInt8(static_cast<uint8_t>(tag.getType()));
    if (tag.getType() != NBTTagType::End) {
      out.writeVarInt(static_cast<int32_t>(name.size()));
      if (!name.empty()) {
        out.writeRaw(name.data(), name.size());
      }
      tag.serialize(out);
    }
  }
};
} // namespace mc::datatypes::nbt
