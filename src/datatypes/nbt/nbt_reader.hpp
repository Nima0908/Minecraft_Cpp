#pragma once
#include "nbt_factory.hpp"
#include "nbt_tag.hpp"
#include <utility>

namespace mc::datatypes::nbt {

class NBTReader {
public:
  static std::pair<std::string, std::unique_ptr<NBTTag>>
  readNamedTag(mc::buffer::ReadBuffer &in) {
    NBTTagType type = static_cast<NBTTagType>(in.readUInt8());
    if (type == NBTTagType::End) {
      return {"", std::make_unique<tags::NBTEnd>()};
    }

    int32_t nameLength = in.readVarInt();
    std::string name;
    if (nameLength > 0) {
      auto nameBytes = in.readBytes(nameLength);
      name = std::string(nameBytes.begin(), nameBytes.end());
    }

    auto tag = createTag(type);
    if (tag) {
      tag->read(in);
    }

    return {name, std::move(tag)};
  }

  static std::unique_ptr<NBTTag> readTag(mc::buffer::ReadBuffer &in,
                                         NBTTagType type) {
    auto tag = createTag(type);
    if (tag) {
      tag->read(in);
    }
    return tag;
  }
};
} // namespace mc::datatypes::nbt
