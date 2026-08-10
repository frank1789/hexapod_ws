#ifndef JOYPAD_H
#define JOYPAD_H

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <unordered_map>

#include "hexapod_msgs/msg/joypad_button.hpp"
#include "hexapod_msgs/msg/joypad_thumbstick.hpp"
#include "hexapod_msgs/msg/joypad_trigger.hpp"
#include "joypad_button.h"
#include "joypad_thumbstick.h"
#include "joypad_trigger.h"

class Joypad : public rclcpp::Node {
 public:
  /**
   * @brief Construct the Joypad node, subscribing to /joy and advertising the
   * remapped joypad/{button,thumbstick,trigger} topics.
   */
  Joypad();

  /**
   * @brief Destroy the Joypad object
   *
   */
  ~Joypad() override = default;

 private:
  void controllerCallback(const sensor_msgs::msg::Joy::ConstSharedPtr& msg);

  // Declared largest first so the node carries no avoidable padding.
  ThumbStick L3_thumbstick_;
  ThumbStick R3_thumbstick_;
  std::unordered_map<int, Button> buttons_;
  Trigger L2_triggers_;
  Trigger R2_triggers_;

  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_subscriber_;
  rclcpp::Publisher<hexapod_msgs::msg::JoypadTrigger>::SharedPtr trigger_publisher_;
  rclcpp::Publisher<hexapod_msgs::msg::JoypadThumbstick>::SharedPtr thumbstick_publisher_;
  rclcpp::Publisher<hexapod_msgs::msg::JoypadButton>::SharedPtr button_publisher_;
};

#endif  // JOYPAD_H
