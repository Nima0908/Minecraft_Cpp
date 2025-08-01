#pragma once
#include "../nbt_tag.hpp"
#include <vector>

namespace mc::datatypes::nbt::tags {

class NBTIntArray : public NBTTag {
private:
  std::vector<int32_t> value;

public:
  NBTIntArray() = default;
  NBTIntArray(const std::vector<int32_t> &val) : value(val) {}

  NBTTagType getType() const override { return NBTTagType::IntArray; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTIntArray>(value);
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    out.writeInt32(static_cast<int32_t>(value.size()));
    for (int32_t val : value) {
      out.writeInt32(val);
    }
  }

  void read(mc::buffer::ReadBuffer &in) override {
    int32_t length = in.readInt32();
    value.resize(length);
    for (int32_t i = 0; i < length; ++i) {
      value[i] = in.readInt32();
    }
  }

  std::string toString() const override {
    return "TAG_Int_Array(length=" + std::to_string(value.size()) + ")";
  }

  const std::vector<int32_t> &getValue() const { return value; }
  std::vector<int32_t> &getValue() { return value; }
  void setValue(const std::vector<int32_t> &val) { value = val; }
};

} // namespace mc::datatypes::nbt::tags
