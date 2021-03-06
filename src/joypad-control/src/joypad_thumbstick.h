#ifndef JOYPAD_THUMBSTICK_H
#define JOYPAD_THIMBSTICK_H

#include <iostream>
#include <string>
#include <tuple>

class ThumbStick {
 public:
  explicit ThumbStick(const std::string& name);
  explicit ThumbStick(const std::string& name, double x_axis, double y_axis);

  // setter methods
  void setXaxis(double x_axis);
  void setYaxis(double y_axis);
  void setAxes(double x_axis, double y_axis);

  // getter methods
  std::tuple<double, double> getRawAxesValues() const;
  std::tuple<double, double> getAxesValues() const;
  std::tuple<double, double> getVectorAxisAngle() const;
  double getMagnitude() const;
  double getAngle() const;

  // accessory function
  friend std::ostream& operator<<(std::ostream& os, const ThumbStick& tb);

 private:
  inline double normalize();
  inline double computeMagnitude();
  inline double computeAngle();

 private:
  double raw_x_axis_;
  double raw_y_axis_;
  double x_axis_normalized_;
  double magnitude_;
  double angle_;
  std::string tb_name_;
};

inline std::ostream& operator<<(std::ostream& os, const ThumbStick& tb);

#endif  // JOYPAD_THIMBSTICK_H