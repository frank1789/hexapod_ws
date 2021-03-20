#ifndef JOYPAD_H
#define JOYPAD_H

#include <ros/ros.h>
#include <sensor_msgs/Joy.h>

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

class Joypad {
 public:
  explicit Joypad();

 private:
  static void controllerCallback(const sensor_msgs::Joy& msg);

 private:
  static std::unordered_map<uint8_t, std::string> triggers_;
  static std::unordered_map<uint8_t, std::string> thumbsticks_;
  static std::unordered_map<uint8_t, std::string> buttons_;
  static std::unordered_map<uint8_t, std::string> special_;

  ros::NodeHandle node_handler_;
  ros::Subscriber joy_subriber_;
};

#endif  // JOYPAD_H