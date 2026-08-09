/**
 * @file servocontroller.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Class hold the interface for Servo Controller board based on Adafruit PCA 9685.
 * @version 0.2.0
 * @date 2022-12-04
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef HEXAPOD_SERVO_CONTROLLER_H_
#define HEXAPOD_SERVO_CONTROLLER_H_

#include <cstdint>
#include <rclcpp/rclcpp.hpp>
#include <string>
#include <vector>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "PCA9685_driver.h"
#include "motor.h"

namespace hexapod {

class ServoController : public rclcpp::Node {
 public:
  /**
   * @brief Construct the ServoController node.
   *
   * Reads its parameters, opens both PCA9685 boards, registers the motors
   * described by motors.lua and moves them to their homing position.
   *
   * @throws std::runtime_error if a Lua script is missing or malformed.
   * @throws std::system_error if the I2C bus cannot be opened.
   */
  ServoController();

  /**
   * @brief Destroy the ServoController object
   *
   */
  ~ServoController() override;

  /**
   * @brief RestoreDefaultPosition method places all engines in the default configuration.
   * This invokes a Lua script that assigns each engine the default position. By adjusting the script appropriately,
   * the rest positions can be changed without the need to recompile the entire project.
   * The angles live in config/homing.lua.
   */
  void RestoreDefaultPosition();

  /**
   * @brief PerformTest method controls for each motor the excursion between 0 and 180 degrees.
   *
   */
  void PerformTest();

 private:
  void RegisterMotors();
  void WriteOnMotor(const Motor& motor);
  [[nodiscard]] uint16_t PulseWidth(int angle) const;

  /**
   * @brief Load a Lua script from the package share directory and return one of
   * its global functions.
   *
   * @param t_filename script file name, relative to config/
   * @param t_entry_point name of the global function to return
   * @throws std::runtime_error if the script cannot be loaded, executed, or
   * does not define the requested function.
   */
  sol::protected_function LoadScript(const std::string& t_filename, const std::string& t_entry_point);

  sol::state lua;
  std::string m_config_directory;
  std::vector<Motor> m_motors;
  adafruit::PCA9685 m_servo_driver_left;
  adafruit::PCA9685 m_servo_driver_right;
  double m_freq{0.0};
};

}  // namespace hexapod

#endif  // HEXAPOD_SERVO_CONTROLLER_H_
