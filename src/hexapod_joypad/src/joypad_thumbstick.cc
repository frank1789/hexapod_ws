#include "joypad_thumbstick.h"

#include <cmath>
#include <iomanip>

ThumbStick::ThumbStick(const std::string& name)
    : raw_x_axis_(0.0), raw_y_axis_(0.0), x_axis_normalized_(0.0), magnitude_(0.0), angle_(0.0), tb_name_(name) {}

ThumbStick::ThumbStick(const std::string& name, double x_axis, double y_axis) {
  tb_name_ = name;
  raw_x_axis_ = x_axis;
  raw_y_axis_ = y_axis;
  x_axis_normalized_ = normalize(x_axis);
  magnitude_ = computeMagnitude(x_axis_normalized_, y_axis);
  angle_ = computeAngle(x_axis_normalized_, y_axis);
}

void ThumbStick::setXaxis(double x_axis) {
  raw_x_axis_ = x_axis;
  x_axis_normalized_ = normalize(x_axis);
}

void ThumbStick::setYaxis(double y_axis) {
  // prefer positive 0.0
  if (y_axis == -0.0 || y_axis == -0) {
    y_axis = std::abs(y_axis);
  }
  raw_y_axis_ = y_axis;
}

void ThumbStick::setAxes(double x_axis, double y_axis) {
  // prefer positive 0.0
  if (y_axis == -0.0 || y_axis == -0) {
    y_axis = std::abs(y_axis);
  }
  raw_x_axis_ = x_axis;
  raw_y_axis_ = y_axis;
  x_axis_normalized_ = normalize(x_axis);
  magnitude_ = computeMagnitude(x_axis_normalized_, y_axis);
  angle_ = computeAngle(x_axis_normalized_, y_axis);
}

void ThumbStick::setName(const std::string& name) { tb_name_ = name; }

std::string ThumbStick::getName() const { return tb_name_; }

std::tuple<double, double> ThumbStick::getRawAxesValues() const { return std::make_tuple(raw_x_axis_, raw_y_axis_); }

std::tuple<double, double> ThumbStick::getAxesValues() const {
  return std::make_tuple(x_axis_normalized_, raw_y_axis_);
}

std::tuple<double, double> ThumbStick::getVectorAxisAngle() const { return std::make_tuple(magnitude_, angle_); }

double ThumbStick::getMagnitude() const { return magnitude_; }

double ThumbStick::getAngle() const { return angle_; }

double ThumbStick::normalize(double axis) {
  axis = (axis * -1);
  if (axis == -0.0 || axis == -0 || axis == 0.0 || axis == 0) {
    axis = std::abs(axis);
  }

  return axis;
}

double ThumbStick::computeMagnitude(double x_axis, double y_axis) { return std::hypot(x_axis, y_axis); }

double ThumbStick::computeAngle(double x_axis, double y_axis) { return std::atan2(y_axis, x_axis); }

std::ostream& operator<<(std::ostream& os, const ThumbStick& tb) {
  os.precision(5);
  return os << tb.tb_name_ << " [" << std::fixed << tb.x_axis_normalized_ << ", " << tb.raw_y_axis_ << "]";
}
