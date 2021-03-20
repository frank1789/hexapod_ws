

#include <ros/ros.h>
#include <ros/console.h>

#include "joypad.h"

int main(int argc, char** argv) {
  ros::init(argc, argv, "controller_node");
  ROS_INFO("start controller_node, starting from now you can controller the hexapod by joypad.");
  Joypad joypad_controller;
  ros::spin();

  return 0;
}