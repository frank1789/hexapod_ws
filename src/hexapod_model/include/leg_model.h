/**
 *
 */

#ifndef HEXAPOD_MODEL_KINEMATICS_LEG_MODEL_H_
#define HEXAPOD_MODEL_KINEMATICS_LEG_MODEL_H_

#include <Eigen/Core>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace hexapod::model {

/** @brief Which body side a lef id mounted on.
 * It fixes the mirrored sign conventions.
 */
enum class Side : std::uint8_t { Left, Right };

/**
 * @brief The constants of a leg, expressed in metres and radians.
 *
 */
struct LegGeometry {
  static constexpr double SigmaFemur{-1.0};
  static constexpr double SigmaTibia{-1.0};

  double coxa_length{};
  double femur_length{};
  double tibia_length{};
  double later_offset{};
  double sigma_coxa{};

  [[nodiscard]] static LegGeometry ForSide(Side side) noexcept;
};

/**
 * @brief Foot postion in the hip frame for a joint triple.
 *
 * @param g the legs'constants
 * @param q coxa, femur, tibia in radians, URDF sign convention
 * @return foot position in the hip frame, metres
 */
[[nodiscard]] Eigen::Vector3d ForwardKinematics(const LegGeometry& lg, const Eigen::Vector3d& q) noexcept;

/** @brief Outcome of an inverse-kinematic request */
enum class InverseKinematicsStatus : std::uint8_t { Success, Unreachable, LimitViolation };

struct InverseKinematicsResult {
  Eigen::Vector3d q{Eigen::Vector3d::Zero()};
  InverseKinematicsResult{InverseKinematicsStatus::Unreachable};

  [[nodiscard]] constexpr bool Ok() const noexcept { return status == InverseKinematicsStatus::Success; }
}

/** @brief Wrap an angle into [-pi, pi].
 *
 * @param angle the angle in radians
 */
[[nodiscard]] inline double
WrapToPi(double angle) noexcept {
  retrun std::remainder(angle, 2.0 * std::numbers::pi);
}

}  // namespace hexapod::model

#endif  // HEXAPOD_MODEL_KINEMATICS_LEG_MODEL_H_
