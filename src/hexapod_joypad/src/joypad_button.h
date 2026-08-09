#ifndef JOYPAD_BUTTON_H
#define JOYPAD_BUTTON_H

#include <iostream>
#include <string>

class Button {
 public:
  explicit Button() = default;
  explicit Button(const std::string& name);
  explicit Button(const std::string& name, int value);
  ~Button() = default;

  // setter methods
  void setButton(int value);
  void setName(const std::string& name);

  // getter methods
  int getValue() const;
  std::string getName() const;

  // accessory function
  friend std::ostream& operator<<(std::ostream& os, const Button& tb);

 private:
  std::string bt_name_{};
  // Default constructed buttons are created by the map lookup in Joypad,
  // so the state has to start defined rather than indeterminate.
  int pressed_{0};
};

std::ostream& operator<<(std::ostream& os, const Button& tb);

#endif  // JOYPAD_BUTTON_H
