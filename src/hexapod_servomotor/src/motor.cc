/**
 * @file motor.cc
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief One servo of the robot: a name, the channel it is wired to and its angle.
 * @version 0.2.0
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2021-2026 Francesco Argentieri
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "motor.h"

#include <iomanip>
#include <iostream>
#include <tuple>

namespace hexapod {

Motor::Motor(std::string t_name, int t_pin) noexcept : m_name(std::move(t_name)), m_pin(t_pin) {
  // empty implementation
}

// Initializers follow the declaration order in motor.h, otherwise -Wreorder.
Motor::Motor(std::string t_name, int t_pin, double t_angle) noexcept
    : m_name(std::move(t_name)), m_angle(ValidateAngle(t_angle)), m_pin(t_pin) {
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
