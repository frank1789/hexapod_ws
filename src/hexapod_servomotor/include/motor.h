/**
 * @file motor.h
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

#ifndef SERVOMOTOR_MOTOR_H_
#define SERVOMOTOR_MOTOR_H_

#include <ostream>
#include <string>

namespace hexapod {

class Motor {
 public:
  /**
   * @brief Construct the Motor object
   *
   */
  Motor() noexcept = default;

  /**
   * @brief Construct the Motor object
   * @param t_name assign a human readable name to the motor
   * @param t_pin assign the pin to the motor (refer pin connected on board)
   */
  explicit Motor(std::string t_name, int t_pin) noexcept;

  /**
   * @brief Construct the Motor object
   * @param t_name assign a human readable name to the motor
   * @param t_pin assign the pin to the motor (refer pin connected on board)
   * @param t_angle assign the angle to the motor
   */
  Motor(std::string t_name, int t_pin, double t_angle) noexcept;

  Motor(Motor&&) = default;
  Motor(const Motor&) = default;

  /**
   * @brief Destroy the Motor object
   *
   */
  ~Motor() = default;

  /**
   * @brief SetNameMotor update the motor's name
   * @param t_name assign a human readable name to the motor
   */
  void SetNameMotor(const std::string& t_name);

  /**
   * @brief SetPinMotor update the motor's pin
   * @param t_pin assign the pin to the motor (refer pin connected on board)
   */
  void SetPinMotor(const int t_pin);

  /**
   * @brief SetAngle update the motor's angle
   * @param t_angle assign the angle to the motor
   */
  void SetAngle(const double t_angle);

  /**
   * @brief Get the Name Motor object
   *
   * @return const std::string&
   */
  [[nodiscard]] const std::string& GetNameMotor() const;

  /**
   * @brief Get the Pin Motor object
   *
   * @return int
   */
  [[nodiscard]] int GetPinMotor() const;

  /**
   * @brief Get the Angle object
   *
   * @return double
   */
  [[nodiscard]] double GetAngle() const;

  friend bool operator==(const Motor& lhs, const Motor& rhs);
  friend bool operator!=(const Motor& lhs, const Motor& rhs);
  friend std::ostream& operator<<(std::ostream& os, const Motor& t_motor);

 private:
  auto Reflect() const;

  static double ValidateAngle(const double);

  std::string m_name{};
  double m_angle{0.0};
  /** @brief -1 marks a motor that has not been assigned a channel yet. */
  int m_pin{-1};
};

/**
 * @brief Compare the equality among to Motor
 *
 * @param lhs the first Motor object
 * @param rhs the second Motor object
 * @return true if equal
 * @return false if not equal
 */
bool operator==(const Motor& lhs, const Motor& rhs);

/**
 * @brief Compare the inequality among to Motor
 *
 * @param lhs the first Motor object
 * @param rhs the second Motor object
 * @return true if not equal
 * @return false if equal
 */
bool operator!=(const Motor& lhs, const Motor& rhs);

/**
 * @brief operator<< return a stream containing engine status information
 *
 * @param os the stream
 * @param t_motor the Motor object
 * @return the stream
 */
std::ostream& operator<<(std::ostream& os, const Motor& t_motor);

}  // namespace hexapod

#endif  // SERVOMOTOR_MOTOR_H_
