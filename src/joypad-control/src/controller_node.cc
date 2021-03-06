

#include <ros/ros.h>

#include "joypad.h"

int main(int argc, char** argv) {
  ros::init(argc, argv, "controller_node");
  Controller c;
  ros::spin();

  return 0;
}