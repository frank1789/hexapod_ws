/**
 * @file hip_mount.h
 * @brief Where a leg is bolted to the body, and the frame change it implies.
 */

#ifndef HEXAPOD_MODEL_HIP_MOUNT_H_
#define HEXAPOD_MODEL_HIP_MOUNT_H_

#include <Eigen/Core>

namespace hexapod::model {

/**
 * @brief The fixed transform between the body frame and one leg's hip frame.
 *
 * The rotation is stored twice, once transposed. The inverse of a rotation is
 * its transpose, and ToHip() is on the hot path of every solved pose — six legs
 * per pose — so it is worth not recomputing.
 */
class HipMount {
 public:
  /// A mount at the body origin with no rotation.
  HipMount() = default;

  /**
   * @param position hip origin expressed in the body frame, metres
   * @param rotation body-to-hip rotation, hip axes expressed in body coordinates
   */
  HipMount(const Eigen::Vector3d& position, const Eigen::Matrix3d& rotation) noexcept
      : position_{position}, rotation_{rotation}, rotation_transpose_{rotation.transpose()} {}

  /**
   * @brief Express a point given in the body frame in this leg's hip frame.
   */
  [[nodiscard]] Eigen::Vector3d ToHip(const Eigen::Vector3d& body) const noexcept {
    return rotation_transpose_ * (body - position_);
  }

  /**
   * @brief Express a point given in this leg's hip frame in the body frame.
   */
  [[nodiscard]] Eigen::Vector3d ToBody(const Eigen::Vector3d& hip) const noexcept {
    return (rotation_ * hip) + position_;
  }

  /// Hip origin in the body frame, metres.
  [[nodiscard]] const Eigen::Vector3d& position() const noexcept { return position_; }

  /// Body-to-hip rotation.
  [[nodiscard]] const Eigen::Matrix3d& rotation() const noexcept { return rotation_; }

 private:
  Eigen::Vector3d position_{Eigen::Vector3d::Zero()};
  Eigen::Matrix3d rotation_{Eigen::Matrix3d::Identity()};
  Eigen::Matrix3d rotation_transpose_{Eigen::Matrix3d::Identity()};
};

}  // namespace hexapod::model

#endif  // HEXAPOD_MODEL_HIP_MOUNT_H_
