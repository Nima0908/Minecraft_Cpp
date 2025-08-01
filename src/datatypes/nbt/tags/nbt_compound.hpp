#pragma once
#include "../nbt_tag.hpp"
#include <map>

namespace mc::datatypes::nbt::tags {

// Forward declaration for factory function
std::unique_ptr<NBTTag> createTag(NBTTagType type);

class NBTCompound : public NBTTag {
private:
  std::map<std::string, std::unique_ptr<NBTTag>> value;

public:
  NBTCompound() = default;

  NBTTagType getType() const override { return NBTTagType::Compound; }

  std::unique_ptr<NBTTag> clone() const override {
    auto cloned = std::make_unique<NBTCompound>();
    for (const auto &[name, tag] : value) {
      cloned->value[name] = tag->clone();
    }
    return cloned;
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    for (const auto &[name, tag] : value) {
      out.writeUInt8(static_cast<uint8_t>(tag->getType()));
      // Write string manually for NBT format
      out.writeVarInt(static_cast<int32_t>(name.size()));
      if (!name.empty()) {
        out.writeRaw(name.data(), name.size());
      }
      tag->serialize(out);
    }
    out.writeUInt8(static_cast<uint8_t>(NBTTagType::End));
  }

  void read(mc::buffer::ReadBuffer &in) override {
    value.clear();

    while (true) {
      NBTTagType type = static_cast<NBTTagType>(in.readUInt8());
      if (type == NBTTagType::End) {
        break;
      }

      // Read string manually for NBT format
      int32_t nameLength = in.readVarInt();
      std::string name;
      if (nameLength > 0) {
        auto nameBytes = in.readBytes(nameLength);
        name = std::string(nameBytes.begin(), nameBytes.end());
      }

      auto tag = createTag(type);
      if (tag) {
        tag->read(in);
        value[name] = std::move(tag);
      }
    }
  }

  std::string toString() const override {
    return "TAG_Compound(entries=" + std::to_string(value.size()) + ")";
  }

  const std::map<std::string, std::unique_ptr<NBTTag>> &getValue() const {
    return value;
  }

  NBTTag *getTag(const std::string &name) const {
    auto it = value.find(name);
    return it != value.end() ? it->second.get() : nullptr;
  }

  void setTag(const std::string &name, std::unique_ptr<NBTTag> tag) {
    value[name] = std::move(tag);
  }

  void removeTag(const std::string &name) { value.erase(name); }

  bool hasTag(const std::string &name) const {
    return value.find(name) != value.end();
  }

  size_t size() const { return value.size(); }
  bool empty() const { return value.empty(); }

  // Iterator support
  auto begin() const { return value.begin(); }
  auto end() const { return value.end(); }
  auto begin() { return value.begin(); }
  auto end() { return value.end(); }
};

} // namespace mc::datatypes::nbt::tags
