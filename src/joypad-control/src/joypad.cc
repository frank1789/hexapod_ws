#include "joypad.h"

#include <ros/console.h>

#include "buttonsmap_ps3joy.h"
#include "buttonsname.h"
#include "joypad_button.h"
#include "joypad_thumbstick.h"
#include "joypad_trigger.h"

const std::unordered_map<uint8_t, std::string> Joypad::triggers_{
    {6, trigger::KL2}, {7, trigger::KR2}};

const std::unordered_map<uint8_t, std::string> Joypad::thumbsticks_{
    {0, thumbstick::kL3},
    {1, thumbstick::kL3},
    {3, thumbstick::kR3},
    {4, thumbstick::kR3}};

const std::unordered_map<uint8_t, std::string> Joypad::buttons_{
    {0, button::kCross},  {1, button::kCircle}, {2, button::kTriangle},
    {3, button::kSquare}, {4, button::kL1},     {5, button::KR1},
    {13, button::kUp},    {14, button::kDown},  {15, button::kRight},
    {16, button::kLeft},
};

const std::unordered_map<uint8_t, std::string> Joypad::special_{
    {8, button::kSelect},
    {9, button::kStart},
    {10, button::kPlaystation},
};

Joypad::Joypad() {
  // Subscribe to the /joy topic for input from joystick
  joy_subriber_ = node_handler_.subscribe<sensor_msgs::Joy>("joy", 1000, &Joypad::controllerCallback, this);
}

void Joypad::performAction(std::function<void(void)> ptr_fun) {}

// std_msgs/Header header
// float32[] axes
// int32[] buttons
void Joypad::controllerCallback(const sensor_msgs::Joy::ConstPtr& msg) {
  // ROS_INFO("Joypad: [%s]", msg->data.c_str());
  // decode two array
//   msg->axes();
//   msg->buttons();
  if(msg->button[PS3_BUTTON_SELECT]){
    ROS_DEBUG("Press %s",     special_[8])
  }
}