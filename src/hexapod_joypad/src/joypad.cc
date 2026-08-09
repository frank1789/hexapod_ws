#include "joypad.h"

#include <cstddef>
#include <string>
#include <tuple>

#include "buttonsmap_ps3joy.h"
#include "buttonsname.h"

namespace {

const std::string topic_btn{"joypad/button"};
const std::string topic_tbs{"joypad/thumbstick"};
const std::string topic_trg{"joypad/trigger"};

constexpr int kQueueDepth{10};

constexpr double kPi = 3.141592653589793238463;
constexpr double radiantToDeg(double angle) { return ((angle * 180) / kPi); }

}  // namespace

Joypad::Joypad() : rclcpp::Node("controller_node") {
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
  joy_subscriber_ = create_subscription<sensor_msgs::msg::Joy>(
      "joy", kQueueDepth, [this](const sensor_msgs::msg::Joy::ConstSharedPtr& msg) { controllerCallback(msg); });

  trigger_publisher_ = create_publisher<hexapod_msgs::msg::JoypadTrigger>(topic_trg, kQueueDepth);
  thumbstick_publisher_ = create_publisher<hexapod_msgs::msg::JoypadThumbstick>(topic_tbs, kQueueDepth);
  button_publisher_ = create_publisher<hexapod_msgs::msg::JoypadButton>(topic_btn, kQueueDepth);
}

void Joypad::controllerCallback(const sensor_msgs::msg::Joy::ConstSharedPtr& msg) {
  RCLCPP_DEBUG_STREAM(get_logger(), "Joypad::controllerCallback");

  // The driver must expose every axis the remap reads, otherwise the indexing
  // below is out of bounds. Fail loudly instead of publishing garbage.
  constexpr std::size_t kRequiredAxes{6};
  if (msg->axes.size() < kRequiredAxes) {
    RCLCPP_ERROR_STREAM(get_logger(), "joy message carries " << msg->axes.size() << " axes, at least " << kRequiredAxes
                                                             << " are required; ignoring it");
    return;
  }

  for (std::size_t i = 0; i < msg->buttons.size(); ++i) {
    const auto index = static_cast<int>(i);
    buttons_[index].setButton(msg->buttons[i]);
    RCLCPP_DEBUG_STREAM(get_logger(), buttons_[index]);
    if (buttons_[index].getValue() != 0) {
      hexapod_msgs::msg::JoypadButton btn_msg;
      btn_msg.button_name = buttons_[index].getName();
      btn_msg.value = buttons_[index].getValue();
      button_publisher_->publish(btn_msg);
    }
  }
  // init msg variables
  hexapod_msgs::msg::JoypadThumbstick tbs_msg_left;
  hexapod_msgs::msg::JoypadThumbstick tbs_msg_right;
  hexapod_msgs::msg::JoypadTrigger tgr_msg_left;
  hexapod_msgs::msg::JoypadTrigger tgr_msg_right;
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
  thumbstick_publisher_->publish(tbs_msg_left);
  thumbstick_publisher_->publish(tbs_msg_right);
  // trigger left and right message
  tgr_msg_left.trigger_name = L2_triggers_.getName();
  tgr_msg_left.value = L2_triggers_.getValue();
  tgr_msg_right.trigger_name = R2_triggers_.getName();
  tgr_msg_right.value = R2_triggers_.getValue();
  // publish triggers
  trigger_publisher_->publish(tgr_msg_left);
  trigger_publisher_->publish(tgr_msg_right);
  // print information
  RCLCPP_DEBUG_STREAM(get_logger(), L3_thumbstick_);
  RCLCPP_DEBUG_STREAM(get_logger(), R3_thumbstick_);
  RCLCPP_DEBUG_STREAM(get_logger(), L2_triggers_);
  RCLCPP_DEBUG_STREAM(get_logger(), R2_triggers_);
}
