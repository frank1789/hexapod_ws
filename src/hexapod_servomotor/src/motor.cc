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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <tuple>

#include "utility_function.h"

namespace hexapod {

namespace {

/** @brief Significant digits used when streaming an angle. */
constexpr int kAnglePrecision{6};

}  // namespace

Motor::Motor(std::string t_name, int t_pin) noexcept : name_(std::move(t_name)), pin_(t_pin) {
  // empty implementation
}

// Initializers follow the declaration order in motor.h, otherwise -Wreorder.
Motor::Motor(std::string t_name, int t_pin, double t_angle) noexcept
    : name_(std::move(t_name)), angle_(ValidateAngle(t_angle)), pin_(t_pin) {
  // empty implementation
}

void Motor::SetNameMotor(const std::string& t_name) {
  if (name_ != t_name) {
    name_ = t_name;
  }
}

void Motor::SetPinMotor(const int t_pin) {
  if (pin_ != t_pin) {
    pin_ = t_pin;
  }
}

void Motor::SetAngle(const double t_angle) { angle_ = ValidateAngle(t_angle); }

const std::string& Motor::GetNameMotor() const { return name_; }

int Motor::GetPinMotor() const { return pin_; }

double Motor::GetAngle() const { return angle_; }

auto Motor::Reflect() const { return std::tie(name_, pin_); }

double Motor::ValidateAngle(const double t_angle) {
  // The travel limits live in utility_function.h; repeating them here is how
  // two copies of the same number drift apart.
  auto angle = std::max(t_angle, kMinAngleDegree);

  if (angle >= kMaxAngleDegree) {
    angle = kMaxAngleDegree - 1.0;
  }

  return angle;
}

bool operator==(const Motor& lhs, const Motor& rhs) { return lhs.Reflect() == rhs.Reflect(); }

bool operator!=(const Motor& lhs, const Motor& rhs) { return !(lhs == rhs); }

std::ostream& operator<<(std::ostream& stream, const Motor& t_motor) {
  return stream << "Motor \"" << t_motor.name_ << "\"\t at pin: " << std::setw(3) << t_motor.pin_
                << " angle: " << std::setprecision(kAnglePrecision) << t_motor.angle_;
}

}  // namespace hexapod
