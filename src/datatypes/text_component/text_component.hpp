#pragma once

#include "../../buffer/read_buffer.hpp"
#include "../../buffer/write_buffer.hpp"
#include "../nbt/nbt_tag.hpp"
#include "click_action_type.hpp"
#include "hover_action_type.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc::datatypes::text_component {

// Forward declarations removed since we define TextComponent first now

class TextComponent {
private:
  std::optional<std::string> text_;
  std::optional<std::string> translate_;
  std::vector<std::unique_ptr<TextComponent>> translateWith_;

  std::optional<std::string> keybind_;
  std::optional<std::string> score_objective_;
  std::optional<std::string> score_name_;
  std::optional<std::string> selector_;

  std::optional<std::string> color_;
  std::optional<std::string> font_;
  std::optional<bool> bold_;
  std::optional<bool> italic_;
  std::optional<bool> underlined_;
  std::optional<bool> strikethrough_;
  std::optional<bool> obfuscated_;

  std::unique_ptr<struct ClickEvent> clickEvent_;
  std::unique_ptr<struct HoverEvent> hoverEvent_;

  std::vector<std::unique_ptr<TextComponent>> extra_;

  std::string clickActionToString(ClickActionType action) const;
  ClickActionType stringToClickAction(const std::string &str) const;
  std::string hoverActionToString(HoverActionType action) const;
  HoverActionType stringToHoverAction(const std::string &str) const;

public:
  TextComponent() = default;
  explicit TextComponent(const std::string &text);

  TextComponent(const TextComponent &other);
  TextComponent &operator=(const TextComponent &other);

  TextComponent(TextComponent &&) = default;
  TextComponent &operator=(TextComponent &&) = default;

  static TextComponent text(const std::string &content);
  static TextComponent translatable(const std::string &key);
  static TextComponent translatable(const std::string &key,
                                    const std::vector<TextComponent> &args);
  static TextComponent keybind(const std::string &key);
  static TextComponent score(const std::string &name,
                             const std::string &objective);
  static TextComponent selector(const std::string &selector);

  TextComponent &setText(const std::string &text);
  TextComponent &setTranslate(const std::string &key);
  TextComponent &addTranslateArg(const TextComponent &arg);
  TextComponent &setKeybind(const std::string &key);
  TextComponent &setScore(const std::string &name,
                          const std::string &objective);
  TextComponent &setSelector(const std::string &selector);

  TextComponent &setColor(const std::string &color);
  TextComponent &setFont(const std::string &font);
  TextComponent &setBold(bool bold = true);
  TextComponent &setItalic(bool italic = true);
  TextComponent &setUnderlined(bool underlined = true);
  TextComponent &setStrikethrough(bool strikethrough = true);
  TextComponent &setObfuscated(bool obfuscated = true);

  TextComponent &setClickEvent(ClickActionType action,
                               const std::string &value);
  TextComponent &setHoverEvent(HoverActionType action,
                               const TextComponent &content);
  TextComponent &setHoverEvent(HoverActionType action,
                               const std::string &value);

  TextComponent &addExtra(const TextComponent &component);
  TextComponent &addExtra(const std::string &text);

  const std::optional<std::string> &getText() const { return text_; }
  const std::optional<std::string> &getTranslate() const { return translate_; }
  const std::vector<std::unique_ptr<TextComponent>> &getTranslateWith() const {
    return translateWith_;
  }

  const std::optional<std::string> &getKeybind() const { return keybind_; }
  const std::optional<std::string> &getColor() const { return color_; }
  const std::optional<bool> &getBold() const { return bold_; }
  const std::optional<bool> &getItalic() const { return italic_; }
  const std::optional<bool> &getUnderlined() const { return underlined_; }
  const std::optional<bool> &getStrikethrough() const { return strikethrough_; }
  const std::optional<bool> &getObfuscated() const { return obfuscated_; }

  const std::vector<std::unique_ptr<TextComponent>> &getExtra() const {
    return extra_;
  }

  bool isSimple() const;
  std::string getPlainText() const;
  TextComponent clone() const;

  void serialize(mc::buffer::WriteBuffer &out) const;
  void deserialize(mc::buffer::ReadBuffer &in);

  std::unique_ptr<mc::datatypes::nbt::NBTTag> toNBT() const;
  static TextComponent fromNBT(const mc::datatypes::nbt::NBTTag &nbt);

  std::string toString() const;
};

struct ClickEvent {
  ClickActionType action;
  std::string value;

  ClickEvent(ClickActionType act, const std::string &val)
      : action(act), value(val) {}
};

struct HoverEvent {
  HoverActionType action;
  std::unique_ptr<TextComponent> contents;
  std::string value;

  HoverEvent(HoverActionType act, std::unique_ptr<TextComponent> content)
      : action(act), contents(std::move(content)) {}

  HoverEvent(HoverActionType act, const std::string &val)
      : action(act), value(val) {}
};

} // namespace mc::datatypes::text_component
