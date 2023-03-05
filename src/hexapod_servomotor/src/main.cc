/**
 * @file servomotors.cc
 * @author Francesco Argentieri (francesco.argentieri89@gmail.com)
 * @brief Entry point for Servo Motors Node.
 * @version 0.1.0
 * @date 2022-12-04
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <ros/console.h>
#include <ros/ros.h>

#include "servocontroller.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

int main(int argc, char** argv) {
  ros::init(argc, argv, "servomotors_node");
  if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME,
                                     ros::console::levels::Debug)) {  // Change the level to fit your needs
    ros::console::notifyLoggerLevelsChanged();
  }

  ROS_INFO("Initialize servomotors node.");
  try {
    hexapod::ServoController hmtc;
    hmtc.PerformTest();
    hmtc.RestoreDefaultPosition();
    ros::spin();
  } catch (const std::runtime_error& re) {
    // specific handling for runtime_error
    std::cerr << "Runtime error: " << re.what() << std::endl;
    return 1;
  } catch (const std::exception& ex) {
    // specific handling for all exceptions extending std::exception, except
    // std::runtime_error which is handled explicitly
    std::cerr << "Error occurred: " << ex.what() << std::endl;
    return 1;
  } catch (...) {
    // catch any other errors (that we have no information about)
    std::cerr << "Unknown failure occurred." << std::endl;
    return 1;
  }

  return 0;
}
