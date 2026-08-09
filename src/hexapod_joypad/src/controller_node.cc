#include <memory>
#include <rclcpp/rclcpp.hpp>

#include "joypad.h"

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<Joypad>();
  RCLCPP_INFO(node->get_logger(), "start controller_node, starting from now you can control the hexapod by joypad.");
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
