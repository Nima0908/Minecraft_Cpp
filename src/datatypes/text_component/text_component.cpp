#include "text_component.hpp"
#include "../nbt/tags/nbt_factory.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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

TextComponent &TextComponent::operator=(const std::string &str) {
  *this = TextComponent::fromJson(str);
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

TextComponent TextComponent::fromJson(const std::string &jsonStr) {
  TextComponent comp;
  try {
    auto j = json::parse(jsonStr);
    if (j.contains("text"))
      comp.text_ = j["text"].get<std::string>();
    if (j.contains("translate"))
      comp.translate_ = j["translate"].get<std::string>();
    if (j.contains("with") && j["with"].is_array()) {
      for (const auto &arg : j["with"]) {
        if (arg.is_string()) {
          comp.extra_.emplace_back(std::make_unique<TextComponent>(
              TextComponent::text(arg.get<std::string>())));
        } else if (arg.is_object()) {
          comp.extra_.emplace_back(
              std::make_unique<TextComponent>(fromJson(arg.dump())));
        }
      }
    }
    if (j.contains("color"))
      comp.color_ = j["color"].get<std::string>();
    if (j.contains("bold"))
      comp.bold_ = j["bold"].get<bool>();
    if (j.contains("italic"))
      comp.italic_ = j["italic"].get<bool>();
    if (j.contains("underlined"))
      comp.underlined_ = j["underlined"].get<bool>();
    if (j.contains("strikethrough"))
      comp.strikethrough_ = j["strikethrough"].get<bool>();
    if (j.contains("obfuscated"))
      comp.obfuscated_ = j["obfuscated"].get<bool>();
  } catch (...) {
    comp.text_ = "[Invalid JSON]";
  }
  return comp;
}

void TextComponent::deserializeJson(mc::buffer::ReadBuffer &in) {
  std::string jsonStr = in.readString();
  *this = fromJson(jsonStr);
}

void TextComponent::deserialize(mc::buffer::ReadBuffer &in) {
    deserializeJson(in); // reuse your JSON deserialization
}

void TextComponent::serialize(mc::buffer::WriteBuffer &out) const {
    std::string jsonStr = "{}"; // default empty JSON
    if (text_) jsonStr = "\"" + *text_ + "\""; // minimal example
    out.writeString(jsonStr);
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

std::string TextComponent::toString() const {
  return "TextComponent(" + getPlainText() + ")";
}

} // namespace mc::datatypes::text_component
