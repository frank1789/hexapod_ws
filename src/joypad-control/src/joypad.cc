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
  L3_thumbstick_.setName(thumbstick::kL3);
  R3_thumbstick_.setName(thumbstick::kR3);
  L2_triggers_.setName(trigger::kL2);
  R2_triggers_.setName(trigger::kR2);
  // init buttons
  buttons_[0] = Button(button::kCross);
  buttons_[1] = Button(button::kCircle);
  buttons_[2] = Button(button::kTriangle);
  buttons_[3] = Button(button::kSquare);
  buttons_[4] = Button(button::kL1);
  buttons_[5] = Button(button::KR1);
  buttons_[13] = Button(button::kUp);
  buttons_[14] = Button(button::kDown);
  buttons_[15] = Button(button::kRight);
  buttons_[16] = Button(button::kLeft);
  buttons_[8] = Button(button::kSelect);
  buttons_[9] = Button(button::kStart);
  buttons_[10] = Button(button::kPlaystation);

  // Subscribe to the /joy topic for input from joystick
  joy_subriber_ = node_handler_.subscribe<sensor_msgs::Joy>(
      "joy", 1, &Joypad::controllerCallback, this);
}

void Joypad::controllerCallback(const sensor_msgs::Joy::ConstPtr& msg) {
  ROS_INFO_STREAM("Joypad::controllerCallback");

  for (int i = 0; i < msg->buttons.size(); i++) {
    buttons_[i].setButton(msg->buttons[i]);
    std::cerr << "enter cycle, pressed btn " << buttons_[i] << std::endl;
  }

  L3_thumbstick_.setAxes(msg->axes[0], msg->axes[1]);
  R3_thumbstick_.setAxes(msg->axes[3], msg->axes[4]);
  L2_triggers_.setValue(msg->axes[2]);
  R2_triggers_.setValue(msg->axes[5]);

  std::cerr << L3_thumbstick_ << std::endl;
  std::cerr << R3_thumbstick_ << std::endl;
  std::cerr << L2_triggers_ << std::endl;
  std::cerr << R2_triggers_ << std::endl;
}