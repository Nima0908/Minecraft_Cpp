#pragma once
#include "../nbt_tag.hpp"
#include <vector>

namespace mc::datatypes::nbt::tags {

class NBTLongArray : public NBTTag {
private:
  std::vector<int64_t> value;

public:
  NBTLongArray() = default;
  NBTLongArray(const std::vector<int64_t> &val) : value(val) {}

  NBTTagType getType() const override { return NBTTagType::LongArray; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTLongArray>(value);
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    out.writeInt32(static_cast<int32_t>(value.size()));
    for (int64_t val : value) {
      out.writeLong(val);
    }
  }

  void read(mc::buffer::ReadBuffer &in) override {
    int32_t length = in.readInt32();
    value.resize(length);
    for (int32_t i = 0; i < length; ++i) {
      value[i] = in.readLong();
    }
  }

  std::string toString() const override {
    return "TAG_Long_Array(length=" + std::to_string(value.size()) + ")";
  }

  const std::vector<int64_t> &getValue() const { return value; }
  std::vector<int64_t> &getValue() { return value; }
  void setValue(const std::vector<int64_t> &val) { value = val; }
};

} // namespace mc::datatypes::nbt::tags
