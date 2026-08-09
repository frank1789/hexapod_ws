#ifndef JOYPAD_THUMBSTICK_H
#define JOYPAD_THUMBSTICK_H

#include <ostream>
#include <string>
#include <tuple>

/**
 * @brief One analogue stick, remapped onto the Cartesian convention.
 *
 * The driver reports the horizontal axis positive to the left; this class
 * inverts it, folds negative zero to positive zero, and derives the magnitude
 * and the angle of the stick vector so that consumers do not each recompute
 * them.
 */
class ThumbStick {
 public:
  ThumbStick() = default;
  explicit ThumbStick(std::string name);
  ThumbStick(std::string name, double x_axis, double y_axis);
  ~ThumbStick() = default;

  ThumbStick(const ThumbStick&) = default;
  ThumbStick(ThumbStick&&) noexcept = default;
  ThumbStick& operator=(const ThumbStick&) = default;
  ThumbStick& operator=(ThumbStick&&) noexcept = default;

  // setter methods
  void setXaxis(double x_axis);
  void setYaxis(double y_axis);
  void setAxes(double x_axis, double y_axis);
  void setName(const std::string& name);

  // getter methods
  [[nodiscard]] std::string getName() const;
  [[nodiscard]] std::tuple<double, double> getRawAxesValues() const;
  [[nodiscard]] std::tuple<double, double> getAxesValues() const;
  [[nodiscard]] std::tuple<double, double> getVectorAxisAngle() const;
  [[nodiscard]] double getMagnitude() const;
  [[nodiscard]] double getAngle() const;

  // accessory function
  friend std::ostream& operator<<(std::ostream& stream, const ThumbStick& stick);

 private:
  static double normalize(double axis);
  static double computeMagnitude(double x_axis, double y_axis);
  static double computeAngle(double x_axis, double y_axis);

  // Declared largest first so the object carries no avoidable padding.
  std::string tb_name_;
  double raw_x_axis_{0.0};
  double raw_y_axis_{0.0};
  double x_axis_normalized_{0.0};
  double magnitude_{0.0};
  double angle_{0.0};
};

std::ostream& operator<<(std::ostream& stream, const ThumbStick& stick);

#endif  // JOYPAD_THUMBSTICK_H
