#pragma once
#include "../nbt_tag.hpp"
#include <vector>

namespace mc::datatypes::nbt::tags {

class NBTByteArray : public NBTTag {
private:
  std::vector<int8_t> value;

public:
  NBTByteArray() = default;
  NBTByteArray(const std::vector<int8_t> &val) : value(val) {}

  NBTTagType getType() const override { return NBTTagType::ByteArray; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTByteArray>(value);
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    out.writeInt32(static_cast<int32_t>(value.size()));
    for (int8_t byte : value) {
      out.writeInt8(byte);
    }
  }

  void read(mc::buffer::ReadBuffer &in) override {
    int32_t length = in.readInt32();
    value.resize(length);
    for (int32_t i = 0; i < length; ++i) {
      value[i] = in.readInt8();
    }
  }

  std::string toString() const override {
    return "TAG_Byte_Array(length=" + std::to_string(value.size()) + ")";
  }

  const std::vector<int8_t> &getValue() const { return value; }
  std::vector<int8_t> &getValue() { return value; }
  void setValue(const std::vector<int8_t> &val) { value = val; }
};

} // namespace mc::datatypes::nbt::tags
