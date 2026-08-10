/**
 * @file test_messages.cpp
 * @brief Checks that the generated interfaces exist and keep their field names.
 *
 * The value of these tests is the field names: they are the contract between
 * the joypad remapper and everything that consumes it, so a rename has to break
 * something here rather than silently break a subscriber.
 *
 * @copyright Copyright (c) 2021-2026 Francesco Argentieri
 *
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>

#include "hexapod_msgs/msg/joypad_button.hpp"
#include "hexapod_msgs/msg/joypad_thumbstick.hpp"
#include "hexapod_msgs/msg/joypad_trigger.hpp"

TEST(JoypadButton, DefaultsAreEmpty) {
  const hexapod_msgs::msg::JoypadButton message;
  EXPECT_TRUE(message.button_name.empty());
  EXPECT_EQ(message.value, 0);
}

TEST(JoypadButton, CarriesNameAndValue) {
  hexapod_msgs::msg::JoypadButton message;
  message.button_name = "Cross";
  message.value = 1;

  EXPECT_EQ(message.button_name, "Cross");
  EXPECT_EQ(message.value, 1);
}

TEST(JoypadTrigger, CarriesNameAndTravel) {
  hexapod_msgs::msg::JoypadTrigger message;
  message.trigger_name = "L2";
  message.value = 0.5;

  EXPECT_EQ(message.trigger_name, "L2");
  EXPECT_DOUBLE_EQ(message.value, 0.5);
}

TEST(JoypadThumbstick, DefaultsAreZeroed) {
  const hexapod_msgs::msg::JoypadThumbstick message;
  EXPECT_TRUE(message.thumbstick_name.empty());
  EXPECT_DOUBLE_EQ(message.x_axis, 0.0);
  EXPECT_DOUBLE_EQ(message.y_axis, 0.0);
  EXPECT_DOUBLE_EQ(message.vector_magnitute, 0.0);
  EXPECT_DOUBLE_EQ(message.vector_angle_rad, 0.0);
  EXPECT_DOUBLE_EQ(message.vector_angle_degree, 0.0);
}

TEST(JoypadThumbstick, CarriesTheWholeVector) {
  hexapod_msgs::msg::JoypadThumbstick message;
  message.thumbstick_name = "L3";
  message.x_axis = 1.0;
  message.y_axis = 0.0;
  message.vector_magnitute = 1.0;
  message.vector_angle_rad = 0.0;
  message.vector_angle_degree = 0.0;

  EXPECT_EQ(message.thumbstick_name, "L3");
  EXPECT_DOUBLE_EQ(message.x_axis, 1.0);
  EXPECT_DOUBLE_EQ(message.vector_magnitute, 1.0);
}
