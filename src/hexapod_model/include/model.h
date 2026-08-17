/**
 *
 */

#include <Eigen/Dense>
#include <array>

namespace hexapod {
namespace model {

Eigen::Vector3d ForwardKinematics(const LegGeometry& lg, const Eigen::Vector3d& q) noexcept;

LegState ForwardKinematicsWithJacobian(const LegGeometry& lg, const Eigen::Vector3d& q) noexcept;

Result InverseKinematics(const LegGeometry& lg, const Eigen::Vector3d& target) noexcept;

class HexapodModel {
 public:
  HexapodModel();

  bool SolveJoints(const BodyPose& bdp, const std::array<Eigen::Vector3d, 6>& feet_world), const std::array<Eigen::Vector3d, 6>* q, std::array<InverseKinematicsStatus, 6>* status) const noexcept;

  void FeetFromJoints(const BodyPose& bdp, const std::array<Eigen::Vector3d, 6>& q,
                      const std::array<Eigen::Vector3d, 6>* status) const noexcept,

      double StabilityMargin(const Eigen::Vector3d& com_world, std::span<std::size_t> supporting) const noexcept;
};

}  // namespace model
}  // namespace hexapod
