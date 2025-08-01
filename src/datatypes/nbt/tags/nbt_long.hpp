#pragma once
#include "../nbt_tag.hpp"

namespace mc::datatypes::nbt::tags {

class NBTLong : public NBTTag {
private:
  int64_t value;

public:
  NBTLong(int64_t val = 0) : value(val) {}

  NBTTagType getType() const override { return NBTTagType::Long; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTLong>(value);
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    out.writeLong(value);
  }

  void read(mc::buffer::ReadBuffer &in) override { value = in.readLong(); }

  std::string toString() const override {
    return "TAG_Long(" + std::to_string(value) + ")";
  }

  int64_t getValue() const { return value; }
  void setValue(int64_t val) { value = val; }
};

} // namespace mc::datatypes::nbt::tags
