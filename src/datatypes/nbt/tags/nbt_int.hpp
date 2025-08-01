#pragma once
#include "../nbt_tag.hpp"

namespace mc::datatypes::nbt::tags {

class NBTInt : public NBTTag {
private:
  int32_t value;

public:
  NBTInt(int32_t val = 0) : value(val) {}

  NBTTagType getType() const override { return NBTTagType::Int; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTInt>(value);
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    out.writeInt32(value);
  }

  void read(mc::buffer::ReadBuffer &in) override { value = in.readInt32(); }

  std::string toString() const override {
    return "TAG_Int(" + std::to_string(value) + ")";
  }

  int32_t getValue() const { return value; }
  void setValue(int32_t val) { value = val; }
};

} // namespace mc::datatypes::nbt::tags
