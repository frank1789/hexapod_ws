#include "joypad.h"

#include <ros/console.h>
#include <sensor_msgs/JointState.h>
#include <sensor_msgs/JoyFeedbackArray.h>

#include <iostream>
#include <sstream>
#include <string>

#include "buttonsmap_ps3joy.h"
#include "buttonsname.h"

std::unordered_map<int, Trigger> Joypad::triggers_{{6, Trigger(trigger::KL2)},
                                                   {7, Trigger(trigger::KR2)}};

std::unordered_map<int, ThumbStick> Joypad::thumbsticks_{{0, ThumbStick(thumbstick::kL3)},
                                                         {1, ThumbStick(thumbstick::kL3)},
                                                         {3, ThumbStick(thumbstick::kR3)},
                                                         {4, ThumbStick(thumbstick::kR3)}};

std::unordered_map<int, Button> Joypad::buttons_{
    {0, Button(button::kCross)},        {1, Button(button::kCircle)},
    {2, Button(button::kTriangle)},     {3, Button(button::kSquare)},
    {4, Button(button::kL1)},           {5, Button(button::KR1)},
    {13, Button(button::kUp)},          {14, Button(button::kDown)},
    {15, Button(button::kRight)},       {16, Button(button::kLeft)},
    {8, Button(button::kSelect)},       {9, Button(button::kStart)},
    {10, Button(button::kPlaystation)},
};

Joypad::Joypad() {
  // Subscribe to the /joy topic for input from joystick
  joy_subriber_ = node_handler_.subscribe<sensor_msgs::Joy>(
      "joy", 10, &Joypad::controllerCallback);
}

void Joypad::controllerCallback(const sensor_msgs::Joy::ConstPtr& msg) {
  ROS_INFO_STREAM("Joypad::controllerCallback");

  for (int i = 0; msg->buttons.size(); i++) {
    auto btn = Joypad::buttons_[i];
    btn.setButton(msg->buttons[i]);
    ROS_DEBUG_STREAM(btn);
  }

  // for(const auto a: msg->axes){
  //   std::cerr << "axes:\t" << a << "\n";
  // }

  // if (msg->axes[PS3_AXIS_STICK_LEFT_UPWARDS]) {
  //   ROS_DEBUG("Press %s", Joypad::thumbsticks_[0].c_str());
  // }
  // if (msg->buttons[PS3_BUTTON_SELECT]) {
  //   ROS_DEBUG("Press %s", Joypad::special_[8].c_str());
  // }
  // if (msg->buttons[PS3_BUTTON_START]) {
  //   ROS_DEBUG("Press %s", Joypad::special_[10].c_str());
  // }
}