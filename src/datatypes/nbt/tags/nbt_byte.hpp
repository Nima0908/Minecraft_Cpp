#pragma once
#include "../nbt_tag.hpp"

namespace mc::datatypes::nbt::tags {

class NBTByte : public NBTTag {
private:
  int8_t value;

public:
  NBTByte(int8_t val = 0) : value(val) {}

  NBTTagType getType() const override { return NBTTagType::Byte; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTByte>(value);
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    out.writeInt8(value);
  }

  void read(mc::buffer::ReadBuffer &in) override { value = in.readInt8(); }

  std::string toString() const override {
    return "TAG_Byte(" + std::to_string(value) + ")";
  }

  int8_t getValue() const { return value; }
  void setValue(int8_t val) { value = val; }
};

} // namespace mc::datatypes::nbt::tags
