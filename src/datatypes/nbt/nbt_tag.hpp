#pragma once
#include "../../buffer/read_buffer.hpp"
#include "../../buffer/write_buffer.hpp"
#include "nbt_tag_type.hpp"
#include <memory>
#include <string>

namespace mc::datatypes::nbt {

class NBTTag {
public:
  virtual ~NBTTag() = default;

  virtual NBTTagType getType() const = 0;

  virtual std::unique_ptr<NBTTag> clone() const = 0;

  virtual void serialize(mc::buffer::WriteBuffer &out) const = 0;

  virtual void read(mc::buffer::ReadBuffer &in) = 0;

  virtual std::string toString() const = 0;
};
} // namespace mc::datatypes::nbt
