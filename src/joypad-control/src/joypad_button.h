#ifndef JOYPAD_Button_H
#define JOYPAD_THIMBSTICK_H

#include <iostream>
#include <string>

class Button {
 public:
  explicit Button(const std::string& name);
  explicit Button(const std::string& name, int value);

  // setter methods
  void setButton(int value);

  // getter methods
  int getValue() const;

  // accessory function
  friend std::ostream& operator<<(std::ostream& os, const Button& tb);

 private:
  std::string bt_name_;
  int pressed_;
};

inline std::ostream& operator<<(std::ostream& os, const Button& tb);