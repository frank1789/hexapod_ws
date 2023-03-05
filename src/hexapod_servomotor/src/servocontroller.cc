/**
 * @file servocontoller.cc
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Class hold the interface for Servo Controller board based on Adafruit PCA 9685.
 * @version 0.1.0
 * @date 2022-12-04
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "servocontroller.h"

#include <ros/console.h>

#include "utility_function.h"

namespace hexapod {

constexpr auto kDefaultPWMFreq{50.0}; /**< Default frequency PWM */
constexpr int kReservedMotors{32};    /**< Reserved space for motors */

ServoController::ServoController() : m_servo_driver1("/dev/i2c-1", 0x40), m_servo_driver2("/dev/i2c-1", 0x41) {
  m_motors.reserve(kReservedMotors);
  // open some common LUAlibraries
  lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::debug);
  ROS_DEBUG_STREAM("Initialize ServoController, retrieve params from launch files");
  if (ros::param::has("servomotors/pwm_frequency")) {
    ros::param::get("servomotors/pwm_frequency", m_freq);
    ROS_DEBUG_STREAM("Update default \"PWM frequency\": " << m_freq << " [Hz]");
  } else {
    m_freq = kDefaultPWMFreq;
    ROS_WARN_STREAM("Set to default \"PWM frequency\": " << m_freq << " [Hz]");
  }
  m_servo_driver1.SetPWMFrequency(m_freq);
  m_servo_driver2.SetPWMFrequency(m_freq);

  // initialize motors
  RegisterMotors();
  RestoreDefaultPosition();

  ROS_DEBUG_STREAM("Initialize callbacks and subscribers");
  // m_node_handler.subscribe<>("", , &ServoController:: Callback, this);
  ROS_DEBUG_STREAM("Initialization completed");
}

ServoController::~ServoController() = default;

void ServoController::PerformTest() {
  ROS_INFO_STREAM("=== perform test ===");
  for (auto& motor : m_motors) {
    for (auto i = 0; i < 180; i++) {
      motor.SetAngle(i);
      WriteOnMotor(motor);
    }
    RestoreDefaultPosition();
  }
  ROS_INFO_STREAM("=== end test ===");
  RestoreDefaultPosition();
}

void ServoController::RestoreDefaultPosition() {
  auto script = lua.load_file("/home/pi/hexapod_ws/src/hexapod_servomotor/config/homing.lua");
  if (!script.valid()) {
    sol::error err = script;
    std::cerr << "failed to load string-based script into the program " << err.what() << std::endl;
  }
  script();
  auto get_homing_angle = lua["homing"];
  for (auto& motor : m_motors) {
    auto angle = static_cast<int>(get_homing_angle(motor.GetNameMotor()));
    motor.SetAngle(angle);
    WriteOnMotor(motor);
  }
}

void ServoController::RegisterMotors() {
  auto script = lua.load_file("/home/pi/hexapod_ws/src/hexapod_servomotor/config/motors.lua");
  if (!script.valid()) {
    sol::error err = script;
    std::cerr << "failed to load string-based script into the program " << err.what() << std::endl;
  }
  script();
  auto generator = lua["generate_motors_configuration"];
  for (const auto& side : {"L", "R"}) {
    sol::protected_function_result result = generator(9, side);
    if (!result.valid()) {
      sol::error err = result;
      std::cerr << "failed to load string-based script into the program " << err.what() << std::endl;
    }
  }

  // populate motors
  sol::table motors_table = lua["Motors"];
  auto counter = 0;
  if (motors_table.size() != 0) {
    for (const auto& key_value_pair : motors_table) {
      sol::object key = key_value_pair.first;
      sol::object value = key_value_pair.second;
      sol::type motor_table = value.get_type();
      std::string motor_name{};
      int pin{-1};
      switch (motor_table) {
        case sol::type::table: {
          for (auto const& table : value.as<sol::table>()) {
            switch (table.second.get_type()) {
              case sol::type::string: {
                motor_name = table.second.as<std::string>();
              } break;
              case sol::type::number: {
                pin = table.second.as<int>();
              } break;
              default: {
                std::cout << "hit the default case!" << std::endl;
              }
            }
            ++counter;
          }
        } break;
        case sol::type::string:
          [[fallthrough]];
        case sol::type::number:
          [[fallthrough]];
        default: {
          std::cout << "hit the default case!" << std::endl;
        }
      }

      if (counter % 2 == 0) {
        m_motors.push_back(Motor(motor_name, pin));
        counter = 0;
      }
    }
  }
}

void ServoController::WriteOnMotor(const Motor& motor) {
  if (motor.GetNameMotor().starts_with("L")) {
    m_servo_driver1.SetSinglePWM(motor.GetPinMotor(), 0, PulseWidth(motor.GetAngle()));
  } else {
    m_servo_driver2.SetSinglePWM(motor.GetPinMotor(), 0, PulseWidth(motor.GetAngle()));
  }
  ros::Duration(0.05).sleep();
}

uint16_t ServoController::PulseWidth(int angle) const {
  auto pulse_wide = map(angle, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
  auto analog_value = static_cast<int>((static_cast<double>(pulse_wide) / 1000000 * m_freq * 4096));
  return static_cast<uint16_t>(analog_value);
}

}  // namespace hexapod
