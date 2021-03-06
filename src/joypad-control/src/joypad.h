#ifndef JOYPAD_H
#define JOYPAD_H

#include <ros/ros.h>
#include <sensor_msgs/Joy.h>
#include <std_msgs/Float32.h>
#include <std_msgs/Int32.h>

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

class Joypad {
 public:
  explicit Joypad();
  void performAction(std::function<void(void)> ptr_fun);

 private:
    void controllerCallback(const sensor_msgs::Joy::ConstPtr& msg);


  template <typename T>
  inline T remapValue(T d);

 private:
  static const std::unordered_map<uint8_t, cstring_t> triggers_;
  static const std::unordered_map<uint8_t, cstring_t> thumbsticks_;
  static const std::unordered_map<uint8_t, cstring_t> buttons_;
  static const std::unordered_map<uint8_t, cstring_t> special_;

  ros::NodeHandle node_handler_;
  ros::Subscriber joy_subriber_;
};

#endif  // JOYPAD_H