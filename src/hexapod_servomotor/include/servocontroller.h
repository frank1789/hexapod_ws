/**
 * @file servocontroller.h
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Class hold the interface for Servo Controller board based on Adafruit PCA 9685.
 * @version 0.1.0
 * @date 2022-12-04
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef HEXAPOD_SERVO_CONTROLLER_H_
#define HEXAPOD_SERVO_CONTROLLER_H_

#include <ros/ros.h>

#include <vector>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "PCA9685_driver.h"
#include "motor.h"

namespace hexapod {

class ServoController {
 public:
  /**
   * @brief Construct the ServoController object
   *
   */
  ServoController();

  /**
   * @brief Destroy the ServoController object
   *
   */
  ~ServoController();

  /**
   * @brief RestoreDefaultPosition method places all engines in the default configuration.
   * This invokes an LUA script that assigns each engine the default position. By adjusting the script appropriately,
   * the rest positions can be changed without the need to recompile the entire project.
   *  L_coxaA = 45,
   *  L_femurA = 90,
   *  L_tibiaA = 90,
   *  L_coxaB= 90,
   *  L_femurB = 90,
   *  L_tibiaB = 90,
   *  L_coxaC = 45,
   *  L_femurC = 90,
   *  L_tibiaC = 90,
   *  R_coxaA = 45,
   *  R_femurA = 90,
   *  R_tibiaA = 90,
   *  R_coxaB = 90,
   *  R_femurB = 90,
   *  R_tibiaB = 90,
   *  R_coxaC = 45,
   *  R_femurC = 90,
   *  R_tibiaC = 90
   */
  void RestoreDefaultPosition();

  /**
   * @brief PerformTest method method controls for each motor the excursion between 0 and 180 degrees.
   *
   */
  void PerformTest();

 private:
  void RegisterMotors();
  void WriteOnMotor(const Motor& motor);
  uint16_t PulseWidth(int angle) const;

  ros::NodeHandle m_node_handler;
  ros::Subscriber m_abs_sub;
  ros::Subscriber m_drive_sub;
  sol::state lua;
  std::vector<Motor> m_motors;
  adafruit::PCA9685 m_servo_driver1;
  adafruit::PCA9685 m_servo_driver2;
  double m_freq{0.0};
};

}  // namespace hexapod

#endif  // HEXAPOD_SERVO_CONTROLLER_H_
