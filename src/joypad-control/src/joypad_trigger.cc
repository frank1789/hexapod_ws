#include "joypad_trigger.h"

Trigger::Trigger(const std::string& name)
    : ts_name_(name), raw_value_(0.0), value_(0.0) {}

Trigger::Trigger(const std::string& name, double value) ts_name_(name),
    raw_value_(value), value_(normalize(value_)) {}

void Trigger::setValue(double value) {
  raw_value_ = value;
  value_ = normalize(value_);
}

double Trigger::getValue() const { return value_; }

constexpr double Trigger::normalize(double value) {
  return (value - kMin) / (kMax - kMin);
}

std::ostream& operator<<(std::ostream& os, const Trigger& ts) {
  return os << "trigger " << ts.ts_name_ << " magnitude: " << ts.value_
            << std::endl;
}