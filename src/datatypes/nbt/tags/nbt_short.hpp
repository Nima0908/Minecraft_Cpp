#pragma once
#include "../nbt_tag.hpp"

namespace mc::datatypes::nbt::tags {

class NBTShort : public NBTTag {
private:
  int16_t value;

public:
  NBTShort(int16_t val = 0) : value(val) {}

  NBTTagType getType() const override { return NBTTagType::Short; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTShort>(value);
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    out.writeInt16(value);
  }

  void read(mc::buffer::ReadBuffer &in) override { value = in.readInt16(); }

  std::string toString() const override {
    return "TAG_Short(" + std::to_string(value) + ")";
  }

  int16_t getValue() const { return value; }
  void setValue(int16_t val) { value = val; }
};

} // namespace mc::datatypes::nbt::tags
