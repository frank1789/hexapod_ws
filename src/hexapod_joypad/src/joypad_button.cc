#include "joypad_button.h"

#include <utility>

Button::Button(std::string name) : bt_name_(std::move(name)) {}

Button::Button(std::string name, int value) : bt_name_(std::move(name)), pressed_(value) {}

void Button::setButton(int value) { pressed_ = value; }

void Button::setName(const std::string& name) { bt_name_ = name; }

int Button::getValue() const { return pressed_; }

std::string Button::getName() const { return bt_name_; }

std::ostream& operator<<(std::ostream& stream, const Button& button) {
  const auto* status = button.pressed_ == 1 ? "pressed" : "false";
  return stream << button.bt_name_ << " [" << status << "]";
}
