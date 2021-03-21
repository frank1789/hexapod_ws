#ifndef JOYPAD_H
#define JOYPAD_H

#include <ros/ros.h>
#include <sensor_msgs/Joy.h>

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

#include "joypad_button.h"
#include "joypad_thumbstick.h"
#include "joypad_trigger.h"

class Joypad {
 public:
  explicit Joypad();

 private:
  static void controllerCallback(const sensor_msgs::Joy::ConstPtr& msg);

 private:
  std::unordered_map<int, Trigger> triggers_;
  std::unordered_map<int, ThumbStick> thumbsticks_;
  std::unordered_map<int, Button> buttons_;

  ros::NodeHandle node_handler_;
  ros::Subscriber joy_subriber_;
};

#endif  // JOYPAD_H