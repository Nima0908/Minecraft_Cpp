#pragma once
#include "../nbt_tag.hpp"

namespace mc::datatypes::nbt::tags {

class NBTString : public NBTTag {
private:
  std::string value;

public:
  NBTString(const std::string &val = "") : value(val) {}

  NBTTagType getType() const override { return NBTTagType::String; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTString>(value);
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    // NBT strings use VarInt length prefix, not the built-in writeString
    out.writeVarInt(static_cast<int32_t>(value.size()));
    if (!value.empty()) {
      out.writeRaw(value.data(), value.size());
    }
  }

  void read(mc::buffer::ReadBuffer &in) override {
    int32_t length = in.readVarInt();
    if (length > 0) {
      auto bytes = in.readBytes(length);
      value = std::string(bytes.begin(), bytes.end());
    } else {
      value.clear();
    }
  }

  std::string toString() const override {
    return "TAG_String(\"" + value + "\")";
  }

  const std::string &getValue() const { return value; }
  void setValue(const std::string &val) { value = val; }
};

} // namespace mc::datatypes::nbt::tags
