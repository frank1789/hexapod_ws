/**
 * @file main.cc
 * @brief Entry point of the kinematic model node.
 *
 * The node loads the leg configuration, builds the model and reports what it
 * read. It publishes nothing yet: turning a gait into joint angles on
 * `joint_command` is the work this package exists for and has not been written.
 * Bringing the node up is still worth doing on its own — it is what proves the
 * configuration on the robot is complete and solvable, at start-up rather than
 * halfway through a stride.
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <stdexcept>
#include <string>

#include "configuration_loader.h"
#include "model.h"

namespace {

/// Log the geometry that was actually loaded, so a bad file is obvious.
void ReportConfiguration(const rclcpp::Logger& logger, const hexapod::model::HexapodModel& model) {
  for (std::size_t index = 0; index < hexapod::model::kLegCount; ++index) {
    const auto leg = static_cast<hexapod::model::LegIndex>(index);
    const hexapod::model::LegGeometry& geometry = model.geometry(leg);
    RCLCPP_INFO(logger, "leg %zu: coxa %.4f m, femur %.4f m, tibia %.4f m, sigma_coxa %+.0f", index,
                geometry.coxa_length, geometry.femur_length, geometry.tibia_length, geometry.sigma_coxa);
  }
}

}  // namespace

/**
 * @brief Start the kinematic model node.
 *
 * Every failure is reported and returns non-zero, so a supervisor sees the node
 * fail rather than sit there having quietly given up.
 *
 * @return 0 on a clean shutdown, 1 if the node could not be brought up
 */
int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  const auto logger = rclcpp::get_logger("hexapod_model");

  try {
    auto node = std::make_shared<rclcpp::Node>("hexapod_model_node");

    // The file name is a parameter for the same reason the motor table's is:
    // an alternative chassis is a different script, not a different build.
    const auto script = node->declare_parameter<std::string>("legs_script", "legs.lua");
    const std::string path = hexapod::model::ConfigurationPath(script);

    RCLCPP_INFO(logger, "loading leg configuration from %s", path.c_str());
    const hexapod::model::HexapodModel model{hexapod::model::LoadRobotConfiguration(path)};
    ReportConfiguration(node->get_logger(), model);

    rclcpp::spin(node);
  } catch (const std::invalid_argument& error) {
    RCLCPP_FATAL(logger, "invalid configuration: %s", error.what());
    rclcpp::shutdown();
    return 1;
  } catch (const std::runtime_error& error) {
    RCLCPP_FATAL(logger, "runtime error: %s", error.what());
    rclcpp::shutdown();
    return 1;
  } catch (const std::exception& error) {
    RCLCPP_FATAL(logger, "error occurred: %s", error.what());
    rclcpp::shutdown();
    return 1;
  } catch (...) {
    RCLCPP_FATAL(logger, "unknown failure occurred");
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
