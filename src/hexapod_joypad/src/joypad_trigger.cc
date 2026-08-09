#include "joypad_trigger.h"

#include <iomanip>
#include <utility>

namespace {

/** @brief Decimal places used when streaming a value. */
constexpr int kPrecision{5};

}  // namespace

Trigger::Trigger(std::string name) : ts_name_(std::move(name)) {}

// normalize(value), not normalize(value_): value_ is still uninitialised here.
Trigger::Trigger(std::string name, double value)
    : ts_name_(std::move(name)), raw_value_(value), value_(normalize(value)) {}

void Trigger::setValue(double value) {
  raw_value_ = value;
  value_ = normalize(value);
}

void Trigger::setName(const std::string& name) { ts_name_ = name; }

std::string Trigger::getName() const { return ts_name_; }

double Trigger::getValue() const { return value_; }

// clang-format off
double Trigger::normalize(double value) {
  return (((value - kValueMin) / ((kValueMax - kValueMin) * (kMax - kMin))) + kMin);
}
// clang-format on

std::ostream& operator<<(std::ostream& stream, const Trigger& trigger) {
  stream.precision(kPrecision);
  return stream << "trigger " << trigger.ts_name_ << " magnitude: " << std::fixed << trigger.value_;
}
