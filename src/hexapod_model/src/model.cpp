#include "model.h"

#include <Eigen/Geometry>

namespace hexapod {
namespace model {

/// One standard-DH link transform.
/// @param a      link length along x
/// @param alpha  link twist about x, radians
/// @param d      link offset along z
/// @param theta  joint angle about z, radians
[[nodiscard]] inline Eigen::Isometry3d DenavitHartenberg(double a, double alpha, double d, double theta) {
  Eigen::Isometry3d transform{Eigen::Isometry3d::Identity()};
  transform.rotate(Eigen::AngleAxisd(theta, Eigen::Vector3d::UnitZ()));
  transform.translate(Eigen::Vector3d(0.0, 0.0, d));
  transform.translate(Eigen::Vector3d(a, 0.0, 0.0));
  transform.rotate(Eigen::AngleAxisd(alpha, Eigen::Vector3d::UnitX()));
  return transform;
}

bool HexapodModel::SolveJoints(const BodyPose& body,

                               const std::array<Eigen::Vector3d, 6>& feet_world, std::array<Eigen::Vector3d, 6>* q,
                               std::array<InverseKinematicStatus, 6>* status) const noexcept {
  const Eigen::Matrix3d world_to_body = body.orientation.conjugate().toRotationMatrix();

  bool all_ok = true;
  for (std::size_t i = 0; i < 6; ++i) {
    const Eigen::Vector3d pos_body = world_to_body * (feet_world[i] - body.position);
    const auto res = InverseKinematics(geometry_[i], mounts_[i].ToHip(pos_body));
    (*q)[i] = res.q;
    (*status)[i] = res.status;
    all_ok = all_ok && (res.status == InverseKinematicsStatus::Success);
  }

  return all_ok;
}

}  // namespace model
}  // namespace hexapod
