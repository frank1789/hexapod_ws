/**
 * @file configuration_loader.h
 * @brief Turn `config/legs.lua` into a RobotConfiguration.
 *
 * Lua rather than a header of constants, for the same reason the motor table is
 * Lua (`config/motors.lua`): retuning a chassis, swapping a femur or mirroring a
 * leg is then a change of configuration, not a recompilation. This is the only
 * part of hexapod_model that reads a file or knows about ROS; the kinematics in
 * model.h stay free of both.
 */

#ifndef HEXAPOD_MODEL_CONFIGURATION_LOADER_H_
#define HEXAPOD_MODEL_CONFIGURATION_LOADER_H_

#include <string>

#include "model.h"

namespace hexapod::model {

/**
 * @brief Absolute path of a file in this package's installed config directory.
 *
 * @param filename for example "legs.lua"
 * @return the path under the package share directory
 * @throws std::runtime_error if the package share directory cannot be resolved
 */
[[nodiscard]] std::string ConfigurationPath(const std::string& filename);

/**
 * @brief Execute a Lua leg-configuration script and validate what it returns.
 *
 * The script must define a table named `Legs` with one entry per leg, keyed by
 * the URDF leg names — `L_front`, `L_mid`, `L_back`, `R_front`, `R_mid`,
 * `R_back`. Every field is required and every one is checked: a missing key, a
 * value of the wrong type, a non-positive link length or a sigma that is not
 * ±1 raises rather than quietly leaving a leg at zero. A leg silently left at
 * zero would drive real servos to a pose nobody chose.
 *
 * @param script_path absolute path of the Lua script
 * @return the six legs, in LegIndex order
 * @throws std::runtime_error if the script cannot be loaded, cannot be executed,
 *         or does not describe all six legs completely
 */
[[nodiscard]] RobotConfiguration LoadRobotConfiguration(const std::string& script_path);

}  // namespace hexapod::model

#endif  // HEXAPOD_MODEL_CONFIGURATION_LOADER_H_
