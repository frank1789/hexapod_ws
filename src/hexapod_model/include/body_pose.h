/**
 * @file body_pose.h
 * @brief Position and orientation of the body in the world frame.
 */

#ifndef HEXAPOD_MODEL_BODY_POSE_H_
#define HEXAPOD_MODEL_BODY_POSE_H_

// Eigen/Geometry, not Eigen/Core: Quaterniond is declared in the geometry
// module, and Core alone compiles right up until the member is used.
#include <Eigen/Geometry>

namespace hexapod::model {

/**
 * @brief Where the body is, and which way it is facing.
 *
 * The default is the identity pose: at the world origin, level and facing
 * along +x.
 */
class BodyPose {
 public:
  BodyPose() = default;

  /**
   * @param position body origin in the world frame, metres
   * @param orientation body orientation in the world frame
   */
  BodyPose(const Eigen::Vector3d& position, const Eigen::Quaterniond& orientation) noexcept
      : position_{position}, orientation_{orientation.normalized()} {}

  /// Body origin in the world frame, metres.
  [[nodiscard]] const Eigen::Vector3d& position() const noexcept { return position_; }

  /// Body orientation in the world frame, always normalised.
  [[nodiscard]] const Eigen::Quaterniond& orientation() const noexcept { return orientation_; }

  /**
   * @brief Rotation that takes a world-frame direction into the body frame.
   */
  [[nodiscard]] Eigen::Matrix3d WorldToBody() const noexcept { return orientation_.conjugate().toRotationMatrix(); }

 private:
  Eigen::Vector3d position_{Eigen::Vector3d::Zero()};
  Eigen::Quaterniond orientation_{Eigen::Quaterniond::Identity()};
};

}  // namespace hexapod::model

#endif  // HEXAPOD_MODEL_BODY_POSE_H_
