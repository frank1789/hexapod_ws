/**
 * @file configuration_loader.cc
 * @brief sol2 in front of `config/legs.lua`, with every field checked.
 *
 * The discipline here is the one servocontroller.cc already follows: a Lua table
 * is untyped and unversioned, so every lookup is checked and a failure raises
 * std::runtime_error naming the leg and the field. The original hexapod code
 * printed to stderr and carried on, which drove every motor to angle 0.
 */

#include "configuration_loader.h"

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <sol/sol.hpp>
#include <stdexcept>
#include <string>

namespace hexapod::model {

namespace {

/// The Lua key of each leg, in LegIndex order. These are the URDF leg names.
constexpr std::array<const char*, kLegCount> kLegKeys{"L_front", "L_mid", "L_back", "R_front", "R_mid", "R_back"};

/// Read a required number out of a leg's table, or say precisely what is wrong.
[[nodiscard]] double RequireNumber(const sol::table& leg, const std::string& leg_name, const char* field) {
  const sol::optional<double> value = leg[field];
  if (!value) {
    throw std::runtime_error("legs configuration: leg \"" + leg_name + "\" has no numeric \"" + field + "\"");
  }
  if (!std::isfinite(*value)) {
    throw std::runtime_error("legs configuration: leg \"" + leg_name + "\" has a non-finite \"" + field + "\"");
  }
  return *value;
}

/// Read a required sub-table, or say which one is missing.
[[nodiscard]] sol::table RequireTable(const sol::table& parent, const std::string& leg_name, const char* field) {
  const sol::optional<sol::table> value = parent[field];
  if (!value) {
    throw std::runtime_error("legs configuration: leg \"" + leg_name + "\" has no \"" + field + "\" table");
  }
  return *value;
}

/// The geometry of one leg, with the constraints the maths depends on enforced.
[[nodiscard]] LegGeometry ReadGeometry(const sol::table& leg, const std::string& leg_name) {
  LegGeometry geometry{};
  geometry.coxa_length = RequireNumber(leg, leg_name, "coxa_length");
  geometry.femur_length = RequireNumber(leg, leg_name, "femur_length");
  geometry.tibia_length = RequireNumber(leg, leg_name, "tibia_length");
  geometry.lateral_offset = RequireNumber(leg, leg_name, "lateral_offset");
  geometry.tibia_offset = RequireNumber(leg, leg_name, "tibia_offset");
  geometry.sigma_coxa = RequireNumber(leg, leg_name, "sigma_coxa");
  geometry.joint_min = RequireNumber(leg, leg_name, "joint_min");
  geometry.joint_max = RequireNumber(leg, leg_name, "joint_max");

  // IsUsable() is the same predicate the kinematics guard themselves with;
  // failing here means the caller learns at start-up rather than discovering
  // an Unreachable status for every target later.
  if (!geometry.IsUsable()) {
    throw std::runtime_error("legs configuration: leg \"" + leg_name +
                             "\" is not solvable: femur_length and tibia_length must be positive, sigma_coxa must "
                             "be 1 or -1, and joint_min must be below joint_max");
  }
  return geometry;
}

/// Where the leg is bolted on. The URDF mounts differ only in yaw, so that is
/// what the schema exposes; roll and pitch would need a different convention
/// than "rotate about z" and none is recorded anywhere.
[[nodiscard]] HipMount ReadMount(const sol::table& leg, const std::string& leg_name) {
  const sol::table mount = RequireTable(leg, leg_name, "mount");
  const Eigen::Vector3d position{RequireNumber(mount, leg_name, "x"), RequireNumber(mount, leg_name, "y"),
                                 RequireNumber(mount, leg_name, "z")};
  const double yaw = RequireNumber(mount, leg_name, "yaw");

  const double cos_yaw = std::cos(yaw);
  const double sin_yaw = std::sin(yaw);
  Eigen::Matrix3d rotation{Eigen::Matrix3d::Identity()};
  rotation(0, 0) = cos_yaw;
  rotation(0, 1) = -sin_yaw;
  rotation(1, 0) = sin_yaw;
  rotation(1, 1) = cos_yaw;

  return HipMount{position, rotation};
}

}  // namespace

std::string ConfigurationPath(const std::string& filename) {
  return ament_index_cpp::get_package_share_directory("hexapod_model") + "/config/" + filename;
}

RobotConfiguration LoadRobotConfiguration(const std::string& script_path) {
  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

  auto script = lua.load_file(script_path);
  if (!script.valid()) {
    const sol::error err = script;
    throw std::runtime_error("cannot load Lua script \"" + script_path + "\": " + err.what());
  }

  const sol::protected_function_result executed = script();
  if (!executed.valid()) {
    const sol::error err = executed;
    throw std::runtime_error("cannot execute Lua script \"" + script_path + "\": " + err.what());
  }

  const sol::optional<sol::table> legs = lua["Legs"];
  if (!legs) {
    throw std::runtime_error("\"" + script_path + "\" does not define a \"Legs\" table");
  }

  RobotConfiguration configuration{};
  for (std::size_t index = 0; index < kLegCount; ++index) {
    const std::string leg_name{kLegKeys.at(index)};

    const sol::optional<sol::table> leg = (*legs)[leg_name];
    if (!leg) {
      throw std::runtime_error("\"" + script_path + "\": \"Legs\" has no entry for \"" + leg_name + "\"");
    }

    configuration.at(index).geometry = ReadGeometry(*leg, leg_name);
    configuration.at(index).mount = ReadMount(*leg, leg_name);
  }

  return configuration;
}

}  // namespace hexapod::model
