/**
 * @file motor.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Simple class represents the Motor.
 * @version 0.1.0
 * @date 2023-01-14
 *
 * @copyright Copyright (c) 2023
 *
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

  std::string m_name;
  double m_angle{0.0};
  int m_pin;
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
