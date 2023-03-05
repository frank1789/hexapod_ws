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
  std::string bt_name_;
  int pressed_;
};

std::ostream& operator<<(std::ostream& os, const Button& tb);

#endif  // JOYPAD_BUTTON_H
