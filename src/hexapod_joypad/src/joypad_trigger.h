#ifndef JOYPAD_TRIGGER_H
#define JOYPAD_TRIGGER_H

#include <ostream>
#include <string>

/**
 * @brief One analogue trigger, remapped from the raw axis onto 0 to 1.
 *
 * The driver reports a trigger as an axis running from 1.0 when released to
 * -1.0 when fully pressed; this class turns that into 0.0 to 1.0.
 */
class Trigger {
 public:
  Trigger() = default;
  explicit Trigger(std::string name);
  Trigger(std::string name, double value);
  ~Trigger() = default;

  Trigger(const Trigger&) = default;
  Trigger(Trigger&&) noexcept = default;
  Trigger& operator=(const Trigger&) = default;
  Trigger& operator=(Trigger&&) noexcept = default;

  // setter methods
  void setValue(double value);
  void setName(const std::string& name);

  // getter methods
  [[nodiscard]] std::string getName() const;
  [[nodiscard]] double getValue() const;

  // accessory function
  friend std::ostream& operator<<(std::ostream& stream, const Trigger& trigger);

 private:
  static double normalize(double value);

  static constexpr double kMax{1.0};
  static constexpr double kMin{0.0};
  static constexpr double kValueMax{-1.0};
  static constexpr double kValueMin{1.0};

  // Declared largest first so the object carries no avoidable padding.
  std::string ts_name_;
  double raw_value_{0.0};
  double value_{0.0};
};

std::ostream& operator<<(std::ostream& stream, const Trigger& trigger);

#endif  // JOYPAD_TRIGGER_H
