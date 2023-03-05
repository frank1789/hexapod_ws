#include "joypad_trigger.h"

#include <iomanip>

Trigger::Trigger(const std::string& name) : ts_name_(name), raw_value_(0.0), value_(0.0) {}

Trigger::Trigger(const std::string& name, double value)
    : ts_name_(name), raw_value_(value), value_(normalize(value_)) {}

void Trigger::setValue(double value) {
  raw_value_ = value;
  value_ = normalize(value);
}

void Trigger::setName(const std::string& name) { ts_name_ = name; }

std::string Trigger::getName() const { return ts_name_; }

double Trigger::getValue() const { return value_; }

// clang-format off
double Trigger::normalize(double value) {
  return (((value - kValueMin_) / ((kValueMax_ - kValueMin_) * (kMax_ - kMin_))) + kMin_);
}
// clang-format on

std::ostream& operator<<(std::ostream& os, const Trigger& ts) {
  os.precision(5);
  return os << "trigger " << ts.ts_name_ << " magnitude: " << std::fixed << ts.value_;
}