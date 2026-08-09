/**
 * @file main.cc
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Entry point for Servo Motors Node.
 * @version 0.2.0
 * @date 2022-12-04
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <iostream>
#include <memory>
#include <rclcpp/rclcpp.hpp>

#include "servocontroller.h"

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);

  // Raise verbosity with:  ros2 run hexapod_servomotor hexapod_servomotor_node --ros-args --log-level debug
  try {
    auto node = std::make_shared<hexapod::ServoController>();
    RCLCPP_INFO(node->get_logger(), "Initialize servomotors node.");
    node->PerformTest();
    node->RestoreDefaultPosition();
    rclcpp::spin(node);
  } catch (const std::runtime_error& re) {
    // specific handling for runtime_error
    std::cerr << "Runtime error: " << re.what() << std::endl;
    rclcpp::shutdown();
    return 1;
  } catch (const std::exception& ex) {
    // specific handling for all exceptions extending std::exception, except
    // std::runtime_error which is handled explicitly
    std::cerr << "Error occurred: " << ex.what() << std::endl;
    rclcpp::shutdown();
    return 1;
  } catch (...) {
    // catch any other errors (that we have no information about)
    std::cerr << "Unknown failure occurred." << std::endl;
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
