#ifndef JOYPAD_TRIGGER_H
#define JOYPAD_TRIGGER_H

#include <iostream>
#include <string>

class Trigger {
 public:
  explicit Trigger() = default;
  explicit Trigger(const std::string& name);
  explicit Trigger(const std::string& name, double value);
  ~Trigger() = default;

  // setter methods
  void setValue(double value);
  void setName(const std::string& name);

  // getter methods
  [[nodiscard]] std::string getName() const;
  [[nodiscard]] double getValue() const;

  // accessory function
  friend std::ostream& operator<<(std::ostream& os, const Trigger& ts);

 private:
  double normalize(double value);

 private:
  std::string ts_name_;
  double raw_value_{0.0};
  double value_{0.0};
  static constexpr double kMax_{1.0};
  static constexpr double kMin_{0.0};
  static constexpr double kValueMax_{-1.0};
  static constexpr double kValueMin_{1.0};
};

std::ostream& operator<<(std::ostream& os, const Trigger& ts);

#endif  // JOYPAD_TRIGGER_H
