#include "joypad.h"

#include <ros/console.h>
#include <sensor_msgs/JointState.h>
#include <sensor_msgs/JoyFeedbackArray.h>

#include <string>

#include "buttonsmap_ps3joy.h"
#include "buttonsname.h"
#include "hexapod_msgs/JoypadButton.h"
#include "hexapod_msgs/JoypadThumbstick.h"
#include "hexapod_msgs/JoypadTrigger.h"

const std::string topic_btn{"joypad/button"};
const std::string topic_tbs{"joypad/thumbstick"};
const std::string topic_trg{"joypad/trigger"};

constexpr double kPi = 3.141592653589793238463;
constexpr double radiantToDeg(double angle) { return ((angle * 180) / kPi); }

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
  buttons_[PS3_BUTTON_CROSS_UP] = Button(button::kUp);
  buttons_[PS3_BUTTON_CROSS_DOWN] = Button(button::kDown);
  buttons_[PS3_BUTTON_CROSS_RIGHT] = Button(button::kRight);
  buttons_[PS3_BUTTON_CROSS_LEFT] = Button(button::kLeft);
  buttons_[PS3_BUTTON_SELECT] = Button(button::kSelect);
  buttons_[PS3_BUTTON_START] = Button(button::kStart);
  buttons_[PS3_BUTTON_PAIRING] = Button(button::kPlaystation);
  buttons_[PS3_BUTTON_R3] = Button(thumbstick::kL3);
  buttons_[PS3_BUTTON_L3] = Button(thumbstick::kR3);
  buttons_[PS3_BUTTON_R2] = Button(trigger::kL2);
  buttons_[PS3_BUTTON_L2] = Button(trigger::kR2);

  // Subscribe to the /joy topic for input from joystick
  joy_subscriber_ = node_handler_.subscribe<sensor_msgs::Joy>("joy", 1, &Joypad::controllerCallback, this);

  trigger_publisher_ = node_handler_.advertise<hexapod_msgs::JoypadTrigger>(topic_trg, 1);
  thumbstick_publisher_ = node_handler_.advertise<hexapod_msgs::JoypadThumbstick>(topic_tbs, 1);
  button_publisher_ = node_handler_.advertise<hexapod_msgs::JoypadButton>(topic_btn, 1);
}

void Joypad::controllerCallback(const sensor_msgs::Joy::ConstPtr& msg) {
  ROS_INFO_STREAM("Joypad::controllerCallback");
  for (int i = 0; i < msg->buttons.size(); i++) {
    buttons_[i].setButton(msg->buttons[i]);
    ROS_INFO_STREAM(buttons_[i]);
    if (buttons_[i].getValue() != 0) {
      hexapod_msgs::JoypadButton btn_msg;
      btn_msg.button_name = buttons_[i].getName();
      btn_msg.value = buttons_[i].getValue();
      button_publisher_.publish(btn_msg);
    }
  }
  // init msg variables
  hexapod_msgs::JoypadThumbstick tbs_msg_left;
  hexapod_msgs::JoypadThumbstick tbs_msg_right;
  hexapod_msgs::JoypadTrigger tgr_msg_left;
  hexapod_msgs::JoypadTrigger tgr_msg_right;
  // read/remap raw values from thumbsticks and triggers
  L3_thumbstick_.setAxes(msg->axes[PS3_X_AXIS_L3], msg->axes[PS3_Y_AXIS_L3]);
  R3_thumbstick_.setAxes(msg->axes[PS3_X_AXIS_R3], msg->axes[PS3_Y_AXIS_R3]);
  L2_triggers_.setValue(msg->axes[PS3_TRIGGER_L2]);
  R2_triggers_.setValue(msg->axes[PS3_TRIGGER_R2]);
  // clang-format off
  // send update values left
  tbs_msg_left.thumbstick_name = L3_thumbstick_.getName();
  std::tie(tbs_msg_left.x_axis, tbs_msg_left.y_axis) = L3_thumbstick_.getAxesValues();
  std::tie(tbs_msg_left.vector_magnitute, tbs_msg_left.vector_angle_rad) = L3_thumbstick_.getVectorAxisAngle();
  tbs_msg_left.vector_angle_degree = radiantToDeg(tbs_msg_left.vector_angle_rad);
  // send update values right
  tbs_msg_right.thumbstick_name = R3_thumbstick_.getName();
  std::tie(tbs_msg_right.x_axis, tbs_msg_right.y_axis) = R3_thumbstick_.getAxesValues();
  std::tie(tbs_msg_right.vector_magnitute, tbs_msg_right.vector_angle_rad) = R3_thumbstick_.getVectorAxisAngle();
  tbs_msg_right.vector_angle_degree = radiantToDeg(tbs_msg_right.vector_angle_rad);
  // clang-format on
  // publish
  thumbstick_publisher_.publish(tbs_msg_left);
  thumbstick_publisher_.publish(tbs_msg_right);
  // trigger left and right message
  tgr_msg_left.trigger_name = L2_triggers_.getName();
  tgr_msg_left.value = L2_triggers_.getValue();
  tgr_msg_right.trigger_name = R2_triggers_.getName();
  tgr_msg_right.value = R2_triggers_.getValue();
  // publish triggers
  trigger_publisher_.publish(tgr_msg_left);
  trigger_publisher_.publish(tgr_msg_right);
  // print information
  ROS_INFO_STREAM(L3_thumbstick_);
  ROS_INFO_STREAM(R3_thumbstick_);
  ROS_INFO_STREAM(L2_triggers_);
  ROS_INFO_STREAM(R2_triggers_);
}