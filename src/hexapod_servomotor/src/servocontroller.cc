/**
 * @file servocontroller.cc
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Class hold the interface for Servo Controller board based on Adafruit PCA 9685.
 * @version 0.2.0
 * @date 2022-12-04
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "servocontroller.h"

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <chrono>
#include <stdexcept>
#include <thread>

#include "utility_function.h"

namespace hexapod {

namespace {

constexpr auto kDefaultPWMFreq{50.0};                /**< Default frequency PWM */
constexpr int kReservedMotors{32};                   /**< Reserved space for motors */
constexpr int kMotorsPerSide{9};                     /**< 3 legs per side, 3 joints per leg */
constexpr std::chrono::milliseconds kSettleTime{50}; /**< Time given to a servo to reach its target */

}  // namespace

ServoController::ServoController() : rclcpp::Node("servomotors_node") {
  m_motors.reserve(kReservedMotors);
  // open some common Lua libraries
  lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::debug);

  m_config_directory = ament_index_cpp::get_package_share_directory("hexapod_servomotor") + "/config";
  RCLCPP_DEBUG_STREAM(get_logger(), "Lua configuration directory: " << m_config_directory);

  const auto i2c_bus = declare_parameter<std::string>("i2c_bus", "/dev/i2c-1");
  const auto left_address = declare_parameter<int>("left_driver_address", 0x40);
  const auto right_address = declare_parameter<int>("right_driver_address", 0x41);
  m_freq = declare_parameter<double>("pwm_frequency", kDefaultPWMFreq);
  RCLCPP_DEBUG_STREAM(get_logger(), "PWM frequency: " << m_freq << " [Hz]");

  m_servo_driver_left.Initialize(i2c_bus, static_cast<uint8_t>(left_address));
  m_servo_driver_right.Initialize(i2c_bus, static_cast<uint8_t>(right_address));
  m_servo_driver_left.SetPWMFrequency(m_freq);
  m_servo_driver_right.SetPWMFrequency(m_freq);

  // initialize motors
  RegisterMotors();
  RestoreDefaultPosition();

  RCLCPP_DEBUG_STREAM(get_logger(), "Initialization completed");
}

ServoController::~ServoController() = default;

sol::protected_function ServoController::LoadScript(const std::string& t_filename, const std::string& t_entry_point) {
  const auto path = m_config_directory + "/" + t_filename;

  auto script = lua.load_file(path);
  if (!script.valid()) {
    const sol::error err = script;
    throw std::runtime_error("cannot load Lua script \"" + path + "\": " + err.what());
  }

  const sol::protected_function_result executed = script();
  if (!executed.valid()) {
    const sol::error err = executed;
    throw std::runtime_error("cannot execute Lua script \"" + path + "\": " + err.what());
  }

  sol::protected_function entry_point = lua[t_entry_point];
  if (!entry_point.valid()) {
    throw std::runtime_error("Lua script \"" + path + "\" does not define \"" + t_entry_point + "\"");
  }

  return entry_point;
}

void ServoController::PerformTest() {
  RCLCPP_INFO_STREAM(get_logger(), "=== perform test ===");
  for (auto& motor : m_motors) {
    for (auto i = 0; i < 180; i++) {
      motor.SetAngle(i);
      WriteOnMotor(motor);
    }
    RestoreDefaultPosition();
  }
  RCLCPP_INFO_STREAM(get_logger(), "=== end test ===");
  RestoreDefaultPosition();
}

void ServoController::RestoreDefaultPosition() {
  auto get_homing_angle = LoadScript("homing.lua", "homing");

  for (auto& motor : m_motors) {
    const sol::protected_function_result result = get_homing_angle(motor.GetNameMotor());
    if (!result.valid()) {
      const sol::error err = result;
      throw std::runtime_error("homing() failed for motor \"" + motor.GetNameMotor() + "\": " + err.what());
    }

    const sol::optional<int> angle = result;
    if (!angle.has_value()) {
      throw std::runtime_error("homing.lua defines no angle for motor \"" + motor.GetNameMotor() + "\"");
    }

    motor.SetAngle(*angle);
    WriteOnMotor(motor);
  }
}

void ServoController::RegisterMotors() {
  auto generator = LoadScript("motors.lua", "generate_motors_configuration");

  for (const auto& side : {"L", "R"}) {
    const sol::protected_function_result result = generator(kMotorsPerSide, side);
    if (!result.valid()) {
      const sol::error err = result;
      throw std::runtime_error(std::string("generate_motors_configuration(\"") + side + "\") failed: " + err.what());
    }
  }

  // Each entry of the global Motors table is a {name, pin} pair.
  const sol::optional<sol::table> motors_table = lua["Motors"];
  if (!motors_table.has_value() || motors_table->size() == 0) {
    throw std::runtime_error("motors.lua produced no motor: the global \"Motors\" table is missing or empty");
  }

  for (const auto& entry : *motors_table) {
    if (entry.second.get_type() != sol::type::table) {
      throw std::runtime_error("motors.lua: every entry of \"Motors\" must be a {name, pin} table");
    }

    auto motor_entry = entry.second.as<sol::table>();
    const sol::optional<std::string> name = motor_entry[1];
    const sol::optional<int> pin = motor_entry[2];
    if (!name.has_value() || !pin.has_value()) {
      throw std::runtime_error("motors.lua: an entry of \"Motors\" is not a valid {name, pin} pair");
    }

    m_motors.emplace_back(*name, *pin);
    RCLCPP_DEBUG_STREAM(get_logger(), "registered " << m_motors.back());
  }
}

void ServoController::WriteOnMotor(const Motor& motor) {
  auto& driver = motor.GetNameMotor().starts_with("L") ? m_servo_driver_left : m_servo_driver_right;
  driver.SetSinglePWM(motor.GetPinMotor(), 0, PulseWidth(static_cast<int>(motor.GetAngle())));
  std::this_thread::sleep_for(kSettleTime);
}

uint16_t ServoController::PulseWidth(int angle) const {
  auto pulse_wide = map(angle, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
  auto analog_value = static_cast<int>((static_cast<double>(pulse_wide) / 1000000 * m_freq * 4096));
  return static_cast<uint16_t>(analog_value);
}

}  // namespace hexapod
