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
  buttons_[PS3_BUTTON_ACTION_CROSS] = Button(button::kCross);
  buttons_[PS3_BUTTON_ACTION_CIRCLE] = Button(button::kCircle);
  buttons_[PS3_BUTTON_ACTION_TRIANGLE] = Button(button::kTriangle);
  buttons_[PS3_BUTTON_ACTION_SQUARE] = Button(button::kSquare);
  buttons_[PS3_BUTTON_L1] = Button(button::kL1);
  buttons_[PS3_BUTTON_R1] = Button(button::KR1);
  buttons_[PS3_AXIS_BUTTON_CROSS_UP] = Button(button::kUp);
  buttons_[PS3_AXIS_BUTTON_CROSS_DOWN] = Button(button::kDown);
  buttons_[PS3_AXIS_BUTTON_CROSS_RIGHT] = Button(button::kRight);
  buttons_[PS3_AXIS_BUTTON_CROSS_LEFT] = Button(button::kLeft);
  buttons_[PS3_BUTTON_SELECT] = Button(button::kSelect);
  buttons_[PS3_BUTTON_START] = Button(button::kStart);
  buttons_[PS3_BUTTON_PAIRING] = Button(button::kPlaystation);
  buttons_[PS3_BUTTON_R3] = Button(thumbstick::kL3);
  buttons_[PS3_BUTTON_L3] = Button(thumbstick::kR3);
  buttons_[PS3_BUTTON_R2] = Button(trigger::kL2);
  buttons_[PS3_BUTTON_L2] = Button(trigger::kR2);

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

  L3_thumbstick_.setAxes(msg->axes[PS3_X_AXIS_L3], msg->axes[PS3_Y_AXIS_L3]);
  R3_thumbstick_.setAxes(msg->axes[PS3_X_AXIS_R3], msg->axes[PS3_Y_AXIS_R3]);
  L2_triggers_.setValue(msg->axes[PS3_TRIGGER_L2]);
  R2_triggers_.setValue(msg->axes[PS3_TRIGGER_R2]);

  std::cerr << L3_thumbstick_ << std::endl;
  std::cerr << R3_thumbstick_ << std::endl;
  std::cerr << L2_triggers_ << std::endl;
  std::cerr << R2_triggers_ << std::endl;
}