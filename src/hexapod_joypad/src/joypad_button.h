#ifndef JOYPAD_BUTTON_H
#define JOYPAD_BUTTON_H

#include <ostream>
#include <string>

/**
 * @brief One button of the joypad: a human readable name and its pressed state.
 */
class Button {
 public:
  Button() = default;
  explicit Button(std::string name);
  Button(std::string name, int value);
  ~Button() = default;

  Button(const Button&) = default;
  Button(Button&&) noexcept = default;
  Button& operator=(const Button&) = default;
  Button& operator=(Button&&) noexcept = default;

  // setter methods
  void setButton(int value);
  void setName(const std::string& name);

  // getter methods
  [[nodiscard]] int getValue() const;
  [[nodiscard]] std::string getName() const;

  // accessory function
  friend std::ostream& operator<<(std::ostream& stream, const Button& button);

 private:
  // Declared largest first so the object carries no avoidable padding.
  std::string bt_name_;
  // Default constructed buttons are created by the map lookup in Joypad,
  // so the state has to start defined rather than indeterminate.
  int pressed_{0};
};

std::ostream& operator<<(std::ostream& stream, const Button& button);

#endif  // JOYPAD_BUTTON_H
