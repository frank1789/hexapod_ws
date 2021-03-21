#include "joypad.h"

#include <ros/console.h>
#include <sensor_msgs/JointState.h>
#include <sensor_msgs/JoyFeedbackArray.h>

#include <iostream>
#include <sstream>
#include <string>

#include "buttonsmap_ps3joy.h"
#include "buttonsname.h"

Joypad::Joypad() {
  triggers_{std::make_pair(6, Trigger(trigger::KL2)), std::make_pair(7, Trigger(trigger::KR2))};

  thumbsticks_{std::make_pair(0, ThumbStick(thumbstick::kL3)),
               std::make_pair(1, ThumbStick(thumbstick::kL3)),
               std::make_pair(3, ThumbStick(thumbstick::kR3)),
               std::make_pair(4, ThumbStick(thumbstick::kR3))};
  buttons_{
      std::make_pair(0, Button(button::kCross)),
      std::make_pair(1, Button(button::kCircle)),
      std::make_pair(2, Button(button::kTriangle)),
      std::make_pair(3, Button(button::kSquare)),
      std::make_pair(4, Button(button::kL1)),
      std::make_pair(5, Button(button::KR1)),
      std::make_pair(13, Button(button::kUp)),
      std::make_pair(14, Button(button::kDown)),
      std::make_pair(15, Button(button::kRight)),
      std::make_pair(16, Button(button::kLeft)),
      std::make_pair(8, Button(button::kSelect)),
      std::make_pair(9, Button(button::kStart)),
      std::make_pair(10, Button(button::kPlaystation)),
  };

  // Subscribe to the /joy topic for input from joystick
  joy_subriber_ = node_handler_.subscribe<sensor_msgs::Joy>(
      "joy", 10, &Joypad::controllerCallback, this);
}

void Joypad::controllerCallback(const sensor_msgs::Joy::ConstPtr& msg) {
  ROS_INFO_STREAM("Joypad::controllerCallback");

  for (int i = 0; msg->buttons.size(); i++) {
    buttons_[i].setButton(msg->buttons[i]);
    std::cerr << "enter cycle, pressed btn " << buttons_[i] << std::endl;
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