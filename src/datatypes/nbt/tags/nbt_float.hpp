#pragma once
#include "../nbt_tag.hpp"
#include <iomanip>
#include <sstream>

namespace mc::datatypes::nbt::tags {

class NBTFloat : public NBTTag {
private:
  float value;

public:
  NBTFloat(float val = 0.0f) : value(val) {}

  NBTTagType getType() const override { return NBTTagType::Float; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTFloat>(value);
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    out.writeFloat(value);
  }

  void read(mc::buffer::ReadBuffer &in) override { value = in.readFloat(); }

  std::string toString() const override {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << value;
    return "TAG_Float(" + oss.str() + ")";
  }

  float getValue() const { return value; }
  void setValue(float val) { value = val; }
};

} // namespace mc::datatypes::nbt::tags
