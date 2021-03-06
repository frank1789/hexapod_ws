

#include <ros/ros.h>

#include "joypad.h"

int main(int argc, char** argv) {
  ros::init(argc, argv, "controller_node");
  Joypad joypad_controller;
  ros::spin();

  return 0;
}