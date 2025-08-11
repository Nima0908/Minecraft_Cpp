#include "text_component.hpp"
#include "../nbt/tags/nbt_factory.hpp"

namespace mc::datatypes::text_component {

TextComponent::TextComponent(const std::string &text) : text_(text) {}

TextComponent::TextComponent(const TextComponent &other)
    : text_(other.text_), translate_(other.translate_),
      keybind_(other.keybind_), score_objective_(other.score_objective_),
      score_name_(other.score_name_), selector_(other.selector_),
      color_(other.color_), font_(other.font_), bold_(other.bold_),
      italic_(other.italic_), underlined_(other.underlined_),
      strikethrough_(other.strikethrough_), obfuscated_(other.obfuscated_) {

  for (const auto &comp : other.translateWith_) {
    translateWith_.emplace_back(std::make_unique<TextComponent>(*comp));
  }
  for (const auto &comp : other.extra_) {
    extra_.emplace_back(std::make_unique<TextComponent>(*comp));
  }
  if (other.clickEvent_) {
    clickEvent_ = std::make_unique<ClickEvent>(*other.clickEvent_);
  }
  if (other.hoverEvent_) {
    if (other.hoverEvent_->contents) {
      hoverEvent_ = std::make_unique<HoverEvent>(
          other.hoverEvent_->action,
          std::make_unique<TextComponent>(*other.hoverEvent_->contents));
    } else {
      hoverEvent_ = std::make_unique<HoverEvent>(other.hoverEvent_->action,
                                                 other.hoverEvent_->value);
    }
  }
}

TextComponent &TextComponent::operator=(const TextComponent &other) {
  if (this != &other) {
    TextComponent temp(other);
    *this = std::move(temp);
  }
  return *this;
}

TextComponent TextComponent::text(const std::string &content) {
  return TextComponent(content);
}

TextComponent TextComponent::translatable(const std::string &key) {
  TextComponent comp;
  comp.translate_ = key;
  return comp;
}

TextComponent
TextComponent::translatable(const std::string &key,
                            const std::vector<TextComponent> &args) {
  TextComponent comp;
  comp.translate_ = key;
  for (const auto &arg : args) {
    comp.translateWith_.emplace_back(std::make_unique<TextComponent>(arg));
  }
  return comp;
}

TextComponent TextComponent::keybind(const std::string &key) {
  TextComponent comp;
  comp.keybind_ = key;
  return comp;
}

TextComponent TextComponent::score(const std::string &name,
                                   const std::string &objective) {
  TextComponent comp;
  comp.score_name_ = name;
  comp.score_objective_ = objective;
  return comp;
}

TextComponent TextComponent::selector(const std::string &selector) {
  TextComponent comp;
  comp.selector_ = selector;
  return comp;
}

TextComponent &TextComponent::setText(const std::string &text) {
  text_ = text;
  return *this;
}

TextComponent &TextComponent::setTranslate(const std::string &key) {
  translate_ = key;
  return *this;
}

TextComponent &TextComponent::addTranslateArg(const TextComponent &arg) {
  translateWith_.emplace_back(std::make_unique<TextComponent>(arg));
  return *this;
}

TextComponent &TextComponent::setKeybind(const std::string &key) {
  keybind_ = key;
  return *this;
}

TextComponent &TextComponent::setScore(const std::string &name,
                                       const std::string &objective) {
  score_name_ = name;
  score_objective_ = objective;
  return *this;
}

TextComponent &TextComponent::setSelector(const std::string &selector) {
  selector_ = selector;
  return *this;
}

TextComponent &TextComponent::setColor(const std::string &color) {
  color_ = color;
  return *this;
}

TextComponent &TextComponent::setFont(const std::string &font) {
  font_ = font;
  return *this;
}

TextComponent &TextComponent::setBold(bool bold) {
  bold_ = bold;
  return *this;
}

TextComponent &TextComponent::setItalic(bool italic) {
  italic_ = italic;
  return *this;
}

TextComponent &TextComponent::setUnderlined(bool underlined) {
  underlined_ = underlined;
  return *this;
}

TextComponent &TextComponent::setStrikethrough(bool strikethrough) {
  strikethrough_ = strikethrough;
  return *this;
}

TextComponent &TextComponent::setObfuscated(bool obfuscated) {
  obfuscated_ = obfuscated;
  return *this;
}

TextComponent &TextComponent::setClickEvent(ClickActionType action,
                                            const std::string &value) {
  clickEvent_ = std::make_unique<ClickEvent>(action, value);
  return *this;
}

TextComponent &TextComponent::setHoverEvent(HoverActionType action,
                                            const TextComponent &content) {
  hoverEvent_ = std::make_unique<HoverEvent>(
      action, std::make_unique<TextComponent>(content));
  return *this;
}

TextComponent &TextComponent::setHoverEvent(HoverActionType action,
                                            const std::string &value) {
  hoverEvent_ = std::make_unique<HoverEvent>(action, value);
  return *this;
}

TextComponent &TextComponent::addExtra(const TextComponent &component) {
  extra_.emplace_back(std::make_unique<TextComponent>(component));
  return *this;
}

TextComponent &TextComponent::addExtra(const std::string &text) {
  extra_.emplace_back(
      std::make_unique<TextComponent>(TextComponent::text(text)));
  return *this;
}

std::string TextComponent::clickActionToString(ClickActionType action) const {
  switch (action) {
  case ClickActionType::OpenUrl:
    return "open_url";
  case ClickActionType::RunCommand:
    return "run_command";
  case ClickActionType::SuggestCommand:
    return "suggest_command";
  case ClickActionType::ChangePage:
    return "change_page";
  case ClickActionType::CopyToClipboard:
    return "copy_to_clipboard";
  default:
    return "open_url";
  }
}

ClickActionType
TextComponent::stringToClickAction(const std::string &str) const {
  if (str == "open_url")
    return ClickActionType::OpenUrl;
  if (str == "run_command")
    return ClickActionType::RunCommand;
  if (str == "suggest_command")
    return ClickActionType::SuggestCommand;
  if (str == "change_page")
    return ClickActionType::ChangePage;
  if (str == "copy_to_clipboard")
    return ClickActionType::CopyToClipboard;
  return ClickActionType::OpenUrl;
}

std::string TextComponent::hoverActionToString(HoverActionType action) const {
  switch (action) {
  case HoverActionType::ShowText:
    return "show_text";
  case HoverActionType::ShowItem:
    return "show_item";
  case HoverActionType::ShowEntity:
    return "show_entity";
  default:
    return "show_text";
  }
}

HoverActionType
TextComponent::stringToHoverAction(const std::string &str) const {
  if (str == "show_text")
    return HoverActionType::ShowText;
  if (str == "show_item")
    return HoverActionType::ShowItem;
  if (str == "show_entity")
    return HoverActionType::ShowEntity;
  return HoverActionType::ShowText;
}

bool TextComponent::isSimple() const {
  return text_.has_value() && !color_.has_value() && !font_.has_value() &&
         !bold_.has_value() && !italic_.has_value() &&
         !underlined_.has_value() && !strikethrough_.has_value() &&
         !obfuscated_.has_value() && !clickEvent_ && !hoverEvent_ &&
         extra_.empty();
}

std::string TextComponent::getPlainText() const {
  std::string result;
  if (text_)
    result += *text_;
  else if (translate_)
    result += *translate_;
  else if (keybind_)
    result += *keybind_;
  else if (score_name_)
    result += *score_name_;
  else if (selector_)
    result += *selector_;
  for (const auto &e : extra_) {
    result += e->getPlainText();
  }
  return result;
}

TextComponent TextComponent::clone() const { return TextComponent(*this); }

// Fully-qualified NBT serialization:
std::unique_ptr<mc::datatypes::nbt::NBTTag> TextComponent::toNBT() const {
  if (isSimple()) {
    return std::make_unique<mc::datatypes::nbt::tags::NBTString>(
        text_.value_or(""));
  }

  auto compound = std::make_unique<mc::datatypes::nbt::tags::NBTCompound>();
  if (text_)
    compound->setTag(
        "text", std::make_unique<mc::datatypes::nbt::tags::NBTString>(*text_));
  if (translate_)
    compound->setTag(
        "translate",
        std::make_unique<mc::datatypes::nbt::tags::NBTString>(*translate_));
  if (keybind_)
    compound->setTag(
        "keybind",
        std::make_unique<mc::datatypes::nbt::tags::NBTString>(*keybind_));
  if (score_name_)
    compound->setTag(
        "score",
        std::make_unique<mc::datatypes::nbt::tags::NBTString>(*score_name_));
  if (selector_)
    compound->setTag(
        "selector",
        std::make_unique<mc::datatypes::nbt::tags::NBTString>(*selector_));
  if (color_)
    compound->setTag(
        "color",
        std::make_unique<mc::datatypes::nbt::tags::NBTString>(*color_));
  if (font_)
    compound->setTag(
        "font", std::make_unique<mc::datatypes::nbt::tags::NBTString>(*font_));
  if (bold_)
    compound->setTag(
        "bold",
        std::make_unique<mc::datatypes::nbt::tags::NBTByte>(*bold_ ? 1 : 0));
  if (italic_)
    compound->setTag(
        "italic",
        std::make_unique<mc::datatypes::nbt::tags::NBTByte>(*italic_ ? 1 : 0));
  if (underlined_)
    compound->setTag("underlined",
                     std::make_unique<mc::datatypes::nbt::tags::NBTByte>(
                         *underlined_ ? 1 : 0));
  if (strikethrough_)
    compound->setTag("strikethrough",
                     std::make_unique<mc::datatypes::nbt::tags::NBTByte>(
                         *strikethrough_ ? 1 : 0));
  if (obfuscated_)
    compound->setTag("obfuscated",
                     std::make_unique<mc::datatypes::nbt::tags::NBTByte>(
                         *obfuscated_ ? 1 : 0));

  if (clickEvent_) {
    auto clickCompound =
        std::make_unique<mc::datatypes::nbt::tags::NBTCompound>();
    clickCompound->setTag("action",
                          std::make_unique<mc::datatypes::nbt::tags::NBTString>(
                              clickActionToString(clickEvent_->action)));
    clickCompound->setTag("value",
                          std::make_unique<mc::datatypes::nbt::tags::NBTString>(
                              clickEvent_->value));
    compound->setTag("clickEvent", std::move(clickCompound));
  }

  if (hoverEvent_) {
    auto hoverCompound =
        std::make_unique<mc::datatypes::nbt::tags::NBTCompound>();
    hoverCompound->setTag("action",
                          std::make_unique<mc::datatypes::nbt::tags::NBTString>(
                              hoverActionToString(hoverEvent_->action)));
    if (hoverEvent_->contents) {
      hoverCompound->setTag("contents", hoverEvent_->contents->toNBT());
    } else {
      hoverCompound->setTag(
          "value", std::make_unique<mc::datatypes::nbt::tags::NBTString>(
                       hoverEvent_->value));
    }
    compound->setTag("hoverEvent", std::move(hoverCompound));
  }

  if (!extra_.empty()) {
    auto extraList = std::make_unique<mc::datatypes::nbt::tags::NBTList>(
        mc::datatypes::nbt::NBTTagType::Compound);
    for (const auto &e : extra_) {
      extraList->addTag(e->toNBT());
    }
    compound->setTag("extra", std::move(extraList));
  }

  if (!translateWith_.empty()) {
    auto withList = std::make_unique<mc::datatypes::nbt::tags::NBTList>(
        mc::datatypes::nbt::NBTTagType::Compound);
    for (const auto &t : translateWith_) {
      withList->addTag(t->toNBT());
    }
    compound->setTag("with", std::move(withList));
  }

  return compound;
}

void TextComponent::serialize(mc::buffer::WriteBuffer &out) const {
  auto nbtTag = toNBT();
  out.writeUInt8(static_cast<uint8_t>(nbtTag->getType()));
  nbtTag->serialize(out);
}

void TextComponent::deserialize(mc::buffer::ReadBuffer &in) {
  auto tagType = static_cast<mc::datatypes::nbt::NBTTagType>(in.readUInt8());
  auto tag = mc::datatypes::nbt::tags::createTag(tagType);
  if (tag) {
    tag->read(in);
    *this = fromNBT(*tag);
  }
}

TextComponent TextComponent::fromNBT(const mc::datatypes::nbt::NBTTag &nbt) {
  using namespace mc::datatypes::nbt;
  if (nbt.getType() == NBTTagType::String) {
    auto str = dynamic_cast<const mc::datatypes::nbt::tags::NBTString *>(&nbt);
    return TextComponent::text(str ? str->getValue() : "");
  }

  if (nbt.getType() == NBTTagType::Compound) {
    auto compound =
        dynamic_cast<const mc::datatypes::nbt::tags::NBTCompound *>(&nbt);
    if (!compound)
      return TextComponent();
    TextComponent comp;

    if (auto tag = dynamic_cast<const mc::datatypes::nbt::tags::NBTString *>(
            compound->getTag("text")))
      comp.text_ = tag->getValue();
    if (auto tag = dynamic_cast<const mc::datatypes::nbt::tags::NBTString *>(
            compound->getTag("translate")))
      comp.translate_ = tag->getValue();
    if (auto tag = dynamic_cast<const mc::datatypes::nbt::tags::NBTString *>(
            compound->getTag("keybind")))
      comp.keybind_ = tag->getValue();
    if (auto tag = dynamic_cast<const mc::datatypes::nbt::tags::NBTString *>(
            compound->getTag("color")))
      comp.color_ = tag->getValue();
    if (auto tag = dynamic_cast<const mc::datatypes::nbt::tags::NBTByte *>(
            compound->getTag("bold")))
      comp.bold_ = tag->getValue() != 0;
    if (auto tag = dynamic_cast<const mc::datatypes::nbt::tags::NBTByte *>(
            compound->getTag("italic")))
      comp.italic_ = tag->getValue() != 0;
    if (auto tag = dynamic_cast<const mc::datatypes::nbt::tags::NBTByte *>(
            compound->getTag("underlined")))
      comp.underlined_ = tag->getValue() != 0;
    if (auto tag = dynamic_cast<const mc::datatypes::nbt::tags::NBTByte *>(
            compound->getTag("strikethrough")))
      comp.strikethrough_ = tag->getValue() != 0;
    if (auto tag = dynamic_cast<const mc::datatypes::nbt::tags::NBTByte *>(
            compound->getTag("obfuscated")))
      comp.obfuscated_ = tag->getValue() != 0;

    return comp;
  }

  return TextComponent();
}

std::string TextComponent::toString() const {
  return "TextComponent(" + getPlainText() + ")";
}

} // namespace mc::datatypes::text_component
