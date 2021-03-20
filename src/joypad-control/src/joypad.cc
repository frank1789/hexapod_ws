#include "joypad.h"

#include <ros/console.h>
#include <sensor_msgs/JointState.h>
#include <sensor_msgs/JoyFeedbackArray.h>

#include <iostream>
#include <string>

#include "buttonsmap_ps3joy.h"
#include "buttonsname.h"
#include "joypad_button.h"
#include "joypad_thumbstick.h"
#include "joypad_trigger.h"

std::unordered_map<uint8_t, std::string> Joypad::triggers_{{6, trigger::KL2},
                                                           {7, trigger::KR2}};

std::unordered_map<uint8_t, std::string> Joypad::thumbsticks_{
    {0, thumbstick::kL3},
    {1, thumbstick::kL3},
    {3, thumbstick::kR3},
    {4, thumbstick::kR3}};

std::unordered_map<uint8_t, std::string> Joypad::buttons_{
    {0, button::kCross},  {1, button::kCircle}, {2, button::kTriangle},
    {3, button::kSquare}, {4, button::kL1},     {5, button::KR1},
    {13, button::kUp},    {14, button::kDown},  {15, button::kRight},
    {16, button::kLeft},
};

std::unordered_map<uint8_t, std::string> Joypad::special_{
    {8, button::kSelect},
    {9, button::kStart},
    {10, button::kPlaystation},
};

Joypad::Joypad() {
  // Subscribe to the /joy topic for input from joystick
  joy_subriber_ = node_handler_.subscribe<sensor_msgs::Joy>(
      "joy", 10, &Joypad::controllerCallback);
}

void Joypad::controllerCallback(const sensor_msgs::Joy::ConstPtr& msg) {
  ROS_INFO_STREAM("Joypad::controllerCallback");

  ROS_INFO_STREAM(msg->axes[PS3_AXIS_BUTTON_REAR_LEFT_2]);


  if(msg->axes[PS3_AXIS_STICK_LEFT_UPWARDS]){
    ROS_DEBUG("Press %s", Joypad::thumbsticks_[0].c_str());
  }
  if (msg->buttons[PS3_BUTTON_SELECT]) {
    ROS_DEBUG("Press %s", Joypad::special_[8].c_str());
  }
  if (msg->buttons[PS3_BUTTON_START]) {
    ROS_DEBUG("Press %s", Joypad::special_[10].c_str());
  }
}