#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../../buffer/read_buffer.hpp"
#include "../../buffer/write_buffer.hpp"
#include "../nbt/nbt_tag.hpp"

namespace mc::datatypes::text_component {

class TextComponent {
public:
  TextComponent() = default;
  explicit TextComponent(const std::string &text);
  TextComponent(const TextComponent &other);
  TextComponent &operator=(const TextComponent &other);
  TextComponent &operator=(const std::string &str);

  static TextComponent text(const std::string &content);
  static TextComponent translatable(const std::string &key);
  static TextComponent translatable(const std::string &key,
                                    const std::vector<TextComponent> &args);
  static TextComponent keybind(const std::string &key);
  static TextComponent score(const std::string &name,
                             const std::string &objective);
  static TextComponent selector(const std::string &selector);

  static TextComponent fromJson(const std::string &jsonStr);
  void deserializeJson(mc::buffer::ReadBuffer &in);

  void serialize(mc::buffer::WriteBuffer &out) const;
  void deserialize(mc::buffer::ReadBuffer &in);
  static TextComponent fromNBT(const mc::datatypes::nbt::NBTTag &nbt);

  std::string toString() const;
  std::string getPlainText() const;
  bool isSimple() const;
  TextComponent clone() const;

private:
  std::optional<std::string> text_;
  std::optional<std::string> translate_;
  std::optional<std::string> keybind_;
  std::optional<std::string> score_objective_;
  std::optional<std::string> score_name_;
  std::optional<std::string> selector_;
  std::optional<std::string> color_;
  std::optional<std::string> font_;
  std::optional<bool> bold_, italic_, underlined_, strikethrough_, obfuscated_;

  struct ClickEvent {
  int action;
  std::string value;

  ClickEvent(int a, const std::string &v) : action(a), value(v) {}
};

struct HoverEvent {
  int action;
  std::unique_ptr<TextComponent> contents;
  std::string value;

  HoverEvent(int a, std::unique_ptr<TextComponent> c)
      : action(a), contents(std::move(c)) {}

  HoverEvent(int a, const std::string &v) : action(a), value(v) {}
};

  std::unique_ptr<ClickEvent> clickEvent_;
  std::unique_ptr<HoverEvent> hoverEvent_;
  std::vector<std::unique_ptr<TextComponent>> extra_;
  std::vector<std::unique_ptr<TextComponent>> translateWith_;
};

} // namespace mc::datatypes::text_component
