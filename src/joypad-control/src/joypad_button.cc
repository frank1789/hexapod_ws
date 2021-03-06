#include "joypad_button.h"

#include <cmath>

Button::Button(const std::string& name) : bt_name_(name), pressed_(0) {}

Button::Button(const std::string& name, int value) {
  bt_name_ = name;
  pressed_ = value;
}

void Button::setButton(int value) { pressed_ = value; }

int Button::getValue() const { return pressed_; }

std::ostream& operator<<(std::ostream& os, const Button& bt) {
  auto stat = bt.pressed_ == 1 ? "pressed" : "false";
  return os << bt.bt_name_ << " [" << stat << "]" << std::endl;
}

#endif  // JOYPAD_THIMBSTICK_H