/**
 * @file leg_model.h
 * @brief Geometry, forward and inverse kinematics of a single leg.
 *
 * The joint triple `q` is always (coxa, femur, tibia) in radians, in the URDF
 * sign convention: each joint is centred on 0 and travels ±1.5708 rad. This is
 * *not* the convention the servos use — `config/motors.lua` runs 0–180° with 90°
 * as the rest pose — and nothing in this workspace bridges the two yet.
 */

#ifndef HEXAPOD_MODEL_KINEMATICS_LEG_MODEL_H_
#define HEXAPOD_MODEL_KINEMATICS_LEG_MODEL_H_

#include <Eigen/Core>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace hexapod::model {

/** @brief Which body side a leg is mounted on.
 * It fixes the mirrored sign conventions.
 */
enum class Side : std::uint8_t { Left, Right };

/**
 * @brief The constants of a leg, expressed in metres and radians.
 *
 * **No measurement of any particular robot appears here.** Every field is
 * zero-initialised, which is a deliberately unusable leg: the values are
 * supplied by configuration at run time, exactly as the motor table is
 * (`config/motors.lua`), so a change of chassis is a change of configuration
 * and not a recompilation. IsUsable() is what stops an unpopulated geometry
 * reaching the trigonometry and returning a pose made of NaNs.
 *
 * The two sigma members are sign conventions rather than measurements — they
 * say which way a joint turns, not how long a link is — and they are still
 * configurable per leg through sigma_coxa.
 */
struct LegGeometry {
  /// Sign of the femur joint relative to the leg's sagittal plane.
  static constexpr double SigmaFemur{-1.0};
  /// Sign of the tibia joint relative to the leg's sagittal plane.
  static constexpr double SigmaTibia{-1.0};

  /// Coxa pivot to femur pivot, along the leg's outward axis.
  double coxa_length{};
  /// Femur pivot to tibia pivot.
  double femur_length{};
  /// Tibia pivot to the foot contact point.
  double tibia_length{};
  /// Sideways displacement of the leg plane from the coxa axis.
  double lateral_offset{};
  /// Constant angle built into the tibia link, added to the tibia joint angle.
  double tibia_offset{};
  /// +1 or -1; mirrors the coxa rotation for the right-hand legs.
  double sigma_coxa{};
  /// Lower travel limit shared by the three joints, radians.
  double joint_min{};
  /// Upper travel limit shared by the three joints, radians.
  double joint_max{};

  /**
   * @brief Whether this geometry has been populated with a solvable leg.
   *
   * Guards against the zero-initialised default reaching the trigonometry,
   * where it would divide by zero and return a pose made of NaNs.
   */
  [[nodiscard]] bool IsUsable() const noexcept {
    return femur_length > 0.0 && tibia_length > 0.0 && (sigma_coxa == 1.0 || sigma_coxa == -1.0) &&
           joint_min < joint_max;
  }
};

/** @brief Wrap an angle into [-pi, pi].
 *
 * @param angle the angle in radians
 * @return the equivalent angle in [-pi, pi]
 */
[[nodiscard]] inline double WrapToPi(double angle) noexcept { return std::remainder(angle, 2.0 * std::numbers::pi); }

/**
 * @brief Foot position in the hip frame for a joint triple.
 *
 * @param lg the leg's constants
 * @param q coxa, femur, tibia in radians, URDF sign convention
 * @return foot position in the hip frame, metres
 */
[[nodiscard]] Eigen::Vector3d ForwardKinematics(const LegGeometry& lg, const Eigen::Vector3d& q) noexcept;

/**
 * @brief Foot position and the 3×3 Jacobian d(foot)/d(q) at the same pose.
 */
struct LegState {
  /// Foot position in the hip frame, metres.
  Eigen::Vector3d foot{Eigen::Vector3d::Zero()};
  /// Column j holds the foot velocity produced by a unit rate on joint j.
  Eigen::Matrix3d jacobian{Eigen::Matrix3d::Zero()};
};

/**
 * @brief ForwardKinematics(), plus the analytic Jacobian at that pose.
 *
 * @param lg the leg's constants
 * @param q coxa, femur, tibia in radians, URDF sign convention
 * @return the foot position and d(foot)/d(q)
 */
[[nodiscard]] LegState ForwardKinematicsWithJacobian(const LegGeometry& lg, const Eigen::Vector3d& q) noexcept;

/** @brief Outcome of an inverse-kinematic request */
enum class InverseKinematicsStatus : std::uint8_t { Success, Unreachable, LimitViolation };

/**
 * @brief What InverseKinematics() worked out, and whether it is usable.
 *
 * `q` is only meaningful when Ok() is true. On LimitViolation it still carries
 * the un-clamped solution, so a caller can report how far out of range it was.
 */
struct InverseKinematicsResult {
  Eigen::Vector3d q{Eigen::Vector3d::Zero()};
  InverseKinematicsStatus status{InverseKinematicsStatus::Unreachable};

  [[nodiscard]] constexpr bool Ok() const noexcept { return status == InverseKinematicsStatus::Success; }
};

/**
 * @brief The joint triple that puts the foot on a target, or why it cannot.
 *
 * This is the exact analytic inverse of ForwardKinematics(). Of the two elbow
 * configurations that reach any given point, it returns the one that bends the
 * same way as the robot's rest pose, so a solved pose never flips the knee.
 *
 * @param lg the leg's constants
 * @param target foot position in the hip frame, metres
 * @return the joints and the status; see InverseKinematicsResult
 */
[[nodiscard]] InverseKinematicsResult InverseKinematics(const LegGeometry& lg, const Eigen::Vector3d& target) noexcept;

}  // namespace hexapod::model

#endif  // HEXAPOD_MODEL_KINEMATICS_LEG_MODEL_H_
