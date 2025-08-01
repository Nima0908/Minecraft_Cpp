#pragma once
#include "../nbt_tag.hpp"

namespace mc::datatypes::nbt::tags {

class NBTEnd : public NBTTag {
public:
  NBTTagType getType() const override { return NBTTagType::End; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTEnd>();
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {}

  void read(mc::buffer::ReadBuffer &in) override {}

  std::string toString() const override { return "TAG_End"; }
};

} // namespace mc::datatypes::nbt::tags
