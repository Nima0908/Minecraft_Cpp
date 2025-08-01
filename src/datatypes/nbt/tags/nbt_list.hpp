#pragma once
#include "../nbt_tag.hpp"
#include <vector>

namespace mc::datatypes::nbt::tags {

// Forward declaration for factory function
std::unique_ptr<NBTTag> createTag(NBTTagType type);

class NBTList : public NBTTag {
private:
  NBTTagType listType;
  std::vector<std::unique_ptr<NBTTag>> value;

public:
  NBTList(NBTTagType type = NBTTagType::End) : listType(type) {}

  NBTTagType getType() const override { return NBTTagType::List; }

  std::unique_ptr<NBTTag> clone() const override {
    auto cloned = std::make_unique<NBTList>(listType);
    cloned->value.reserve(value.size());
    for (const auto &tag : value) {
      cloned->value.push_back(tag->clone());
    }
    return cloned;
  }

  void serialize(mc::buffer::WriteBuffer &out) const override {
    out.writeUInt8(static_cast<uint8_t>(listType));
    out.writeInt32(static_cast<int32_t>(value.size()));
    for (const auto &tag : value) {
      tag->serialize(out);
    }
  }

  void read(mc::buffer::ReadBuffer &in) override {
    listType = static_cast<NBTTagType>(in.readUInt8());
    int32_t length = in.readInt32();
    value.clear();
    value.reserve(length);

    for (int32_t i = 0; i < length; ++i) {
      auto tag = createTag(listType);
      if (tag) {
        tag->read(in);
        value.push_back(std::move(tag));
      }
    }
  }

  std::string toString() const override {
    return "TAG_List(type=" + std::to_string(static_cast<int>(listType)) +
           ", length=" + std::to_string(value.size()) + ")";
  }

  NBTTagType getListType() const { return listType; }
  const std::vector<std::unique_ptr<NBTTag>> &getValue() const { return value; }
  std::vector<std::unique_ptr<NBTTag>> &getValue() { return value; }

  void setListType(NBTTagType type) { listType = type; }
  void addTag(std::unique_ptr<NBTTag> tag) { value.push_back(std::move(tag)); }

  NBTTag *getTag(size_t index) const {
    return index < value.size() ? value[index].get() : nullptr;
  }

  size_t size() const { return value.size(); }
  bool empty() const { return value.empty(); }
};

} // namespace mc::datatypes::nbt::tags
