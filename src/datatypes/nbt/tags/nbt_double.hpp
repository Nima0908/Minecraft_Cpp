#pragma once
#include "../nbt_tag.hpp"
#include <iomanip>
#include <sstream>

namespace mc::datatypes::nbt::tags {

class NBTDouble : public NBTTag {
private:
  double value;

public:
  NBTDouble(double val = 0.0) : value(val) {}

  NBTTagType getType() const override { return NBTTagType::Double; }

  std::unique_ptr<NBTTag> clone() const override {
    return std::make_unique<NBTDouble>(value);
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    out.writeDouble(value);
  }

  void read(mc::buffer::ReadBuffer &in) override { value = in.readDouble(); }

  std::string toString() const override {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(15) << value;
    return "TAG_Double(" + oss.str() + ")";
  }

  double getValue() const { return value; }
  void setValue(double val) { value = val; }
};

} // namespace mc::datatypes::nbt::tags
