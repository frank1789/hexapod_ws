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
  /**
   * @brief Construct the Joypad object
   */
  explicit Joypad();

  /**
   * @brief Destroy the Joypad object
   *
   */
  ~Joypad() = default;

 private:
  void controllerCallback(const sensor_msgs::Joy::ConstPtr& msg);

 private:
  Trigger L2_triggers_;
  Trigger R2_triggers_;
  ThumbStick L3_thumbstick_;
  ThumbStick R3_thumbstick_;
  std::unordered_map<int, Button> buttons_;

  ros::NodeHandle node_handler_;
  ros::Subscriber joy_subscriber_;
  ros::Publisher trigger_publisher_;
  ros::Publisher thumbstick_publisher_;
  ros::Publisher button_publisher_;
};

#endif  // JOYPAD_H
