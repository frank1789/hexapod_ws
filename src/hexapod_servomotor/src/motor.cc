/**
 * @file motor.cc
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Simple class represents the Motor.
 * @version 0.1.0
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "motor.h"

#include <iomanip>
#include <iostream>
#include <tuple>

namespace hexapod {

Motor::Motor(std::string t_name, int t_pin) noexcept : m_name(std::move(t_name)), m_pin(t_pin) {
  // empty implementation
}

Motor::Motor(std::string t_name, int t_pin, double t_angle) noexcept
    : m_name(std::move(t_name)), m_pin(t_pin), m_angle(ValidateAngle(t_angle)) {
  // empty implementation
}

void Motor::SetNameMotor(const std::string& t_name) {
  if (m_name != t_name) {
    m_name = t_name;
  }
}

void Motor::SetPinMotor(const int t_pin) {
  if (m_pin != t_pin) {
    m_pin = t_pin;
  }
}

void Motor::SetAngle(const double t_angle) { m_angle = ValidateAngle(t_angle); }

const std::string& Motor::GetNameMotor() const { return m_name; }

int Motor::GetPinMotor() const { return m_pin; }

double Motor::GetAngle() const { return m_angle; }

auto Motor::Reflect() const { return std::tie(m_name, m_pin); }

double Motor::ValidateAngle(const double t_angle) {
  auto angle = t_angle;
  if (angle < 0.0) {
    angle = 0.0;
  }

  if (angle >= 180.0) {
    angle = 179.0;
  }

  return angle;
}

bool operator==(const Motor& lhs, const Motor& rhs) { return lhs.Reflect() == rhs.Reflect(); }

bool operator!=(const Motor& lhs, const Motor& rhs) { return !(lhs == rhs); }

std::ostream& operator<<(std::ostream& os, const Motor& t_motor) {
  return os << "Motor \"" << t_motor.m_name << "\"\t at pin: " << std::setw(3) << t_motor.m_pin
            << " angle: " << std::setprecision(6) << t_motor.m_angle;
}

}  // namespace hexapod
