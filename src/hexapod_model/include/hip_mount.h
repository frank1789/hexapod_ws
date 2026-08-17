#ifndef HEXAPOD_MODEL_HIP_MOUNT_H_
#define HEXAPOD_MODEL_HIP_MOUNT_H_

namespace hexapod::model {

class HipMount {
 public:
  [[nodiscard]] Eigen::Vector3d ToHip(const Eigen::Vector3d& body) const noexcept;

  [[nodiscard]] Eigen::Vector3d ToBody(const Eigen::Vector3d& hip) const noexcept;

 private:
  Eigen::Vector3d position_{Eigen::Vector3d::Zero()};
  Eigen::Matrix3d rotation_{Eigen::Matrix3d::Identity()};
  Eigen::Matrix3d rotation_transpose_{Eigen::Matrix3d::Identity()};
};
}  // namespace hexapod::model

#endif  // HEXAPOD_MODEL_HIP_MOUNT_H_
