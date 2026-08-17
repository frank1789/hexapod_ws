/**
 * @file model.h
 * @brief The six-legged robot: where its legs are, and what its joints must do.
 *
 * Nothing in this header reads a file, opens a socket or touches ROS. The robot
 * arrives as a RobotConfiguration — see configuration_loader.h for the Lua that
 * produces one — so the tests exercise exactly the maths the robot runs without
 * any of that machinery.
 */

#ifndef HEXAPOD_MODEL_MODEL_H_
#define HEXAPOD_MODEL_MODEL_H_

#include <Eigen/Core>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "body_pose.h"
#include "hip_mount.h"
#include "leg_model.h"

namespace hexapod::model {

/// Number of legs. Not a tunable — it is the robot.
inline constexpr std::size_t kLegCount{6};

/**
 * @brief Index of each leg within every per-leg array below.
 *
 * This ordering follows the joint names in `hexapod_description`'s URDF
 * (`<L|R>_<front|mid|back>_...`) and is what `config/legs.lua` keys on. It
 * deliberately makes **no** claim about the `A`/`B`/`C` suffixes used by
 * `config/motors.lua`: which of those is the front, middle and back leg is not
 * recorded anywhere in this repository, so mapping this enum onto a motor name
 * is work that still has to be done against the hardware.
 */
enum class LegIndex : std::uint8_t {
  LeftFront = 0,
  LeftMiddle = 1,
  LeftBack = 2,
  RightFront = 3,
  RightMiddle = 4,
  RightBack = 5,
};

/// Index a per-leg array with a LegIndex.
[[nodiscard]] constexpr std::size_t ToIndex(LegIndex leg) noexcept { return static_cast<std::size_t>(leg); }

/// A joint triple per leg, radians, URDF sign convention.
using JointArray = std::array<Eigen::Vector3d, kLegCount>;
/// A point per leg, metres.
using FootArray = std::array<Eigen::Vector3d, kLegCount>;
/// An inverse-kinematic outcome per leg.
using StatusArray = std::array<InverseKinematicsStatus, kLegCount>;

/**
 * @brief Everything configuration has to say about one leg.
 */
struct LegConfiguration {
  /// Link lengths, offsets, signs and travel limits.
  LegGeometry geometry{};
  /// Where the leg is bolted to the body.
  HipMount mount{};
};

/// The whole robot, in the order given by LegIndex.
using RobotConfiguration = std::array<LegConfiguration, kLegCount>;

/**
 * @brief The robot's fixed geometry, and the kinematics that follow from it.
 *
 * Immutable once built, so it is safe to share between a planner and a
 * controller without copying.
 */
class HexapodModel {
 public:
  /**
   * @brief Build a model from a configuration.
   *
   * There is no default constructor on purpose: a hexapod with no measurements
   * is not a useful object, and the values belong to configuration rather than
   * to this translation unit.
   *
   * @param configuration the six legs, in LegIndex order
   */
  explicit HexapodModel(RobotConfiguration configuration) noexcept;

  /**
   * @brief Whether every leg was configured with a solvable geometry.
   *
   * Checked once here so callers need not re-check per leg; SolveJoints() still
   * reports Unreachable for an unusable leg rather than returning nonsense.
   */
  [[nodiscard]] bool IsUsable() const noexcept;

  /**
   * @brief Joint angles that put every foot on its target.
   *
   * A leg that cannot reach its target does not stop the others: each leg is
   * solved independently, `status` records what happened to it, and the return
   * value says whether all six succeeded.
   *
   * @param body where the body is in the world
   * @param feet_world the six foot targets in the world frame, metres
   * @param[out] joints the solved joint triples; never null
   * @param[out] status per-leg outcome; never null
   * @return true only if every leg reported Success
   */
  bool SolveJoints(const BodyPose& body, const FootArray& feet_world, JointArray* joints,
                   StatusArray* status) const noexcept;

  /**
   * @brief Where the feet end up in the world for a given set of joint angles.
   *
   * The exact inverse of SolveJoints() whenever every leg reports Success.
   *
   * @param body where the body is in the world
   * @param joints the six joint triples, radians
   * @param[out] feet_world the resulting foot positions; never null
   */
  void FeetFromJoints(const BodyPose& body, const JointArray& joints, FootArray* feet_world) const noexcept;

  /**
   * @brief How far the centre of mass sits inside the support polygon.
   *
   * The polygon is the convex hull of the supporting feet projected onto the
   * ground plane. The margin is the distance to the nearest edge: positive
   * inside, negative outside, so a caller compares it against a threshold
   * instead of asking a separate "is it stable" question.
   *
   * Fewer than three supporting feet enclose no area at all, and three collinear
   * ones enclose none either; both return a negative margin rather than
   * reporting a balanced robot.
   *
   * @param com_world centre of mass in the world frame, metres
   * @param feet_world the six foot positions in the world frame, metres
   * @param supporting indices of the feet currently carrying load
   * @return signed distance to the support polygon boundary, metres
   */
  [[nodiscard]] static double StabilityMargin(const Eigen::Vector3d& com_world, const FootArray& feet_world,
                                              std::span<const std::size_t> supporting) noexcept;

  /// The geometry of one leg.
  [[nodiscard]] const LegGeometry& geometry(LegIndex leg) const noexcept {
    return configuration_.at(ToIndex(leg)).geometry;
  }

  /// Where one leg is bolted to the body.
  [[nodiscard]] const HipMount& mount(LegIndex leg) const noexcept { return configuration_.at(ToIndex(leg)).mount; }

 private:
  RobotConfiguration configuration_{};
};

}  // namespace hexapod::model

#endif  // HEXAPOD_MODEL_MODEL_H_
