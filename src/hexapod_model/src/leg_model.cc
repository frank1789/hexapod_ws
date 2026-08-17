/**
 * @file leg_model.cc
 * @brief Forward kinematics, its Jacobian, and the analytic inverse.
 *
 * The leg is a coxa yaw joint carrying a planar two-link arm. Everything below
 * follows from that one picture, so the three functions cannot drift apart:
 * InverseKinematics() inverts exactly what ForwardKinematics() computes, and
 * ForwardKinematicsWithJacobian() differentiates it.
 */

#include "leg_model.h"

#include <algorithm>
#include <cmath>

namespace hexapod::model {

namespace {

/// Angles the leg makes in its own frame, shared by every function here.
struct LegAngles {
  double coxa{};   ///< yaw about the hip z axis
  double femur{};  ///< femur elevation in the leg's sagittal plane
  double tibia{};  ///< tibia elevation in the same plane, femur offset included
};

/// Apply the sign conventions that map joint angles onto frame angles.
[[nodiscard]] LegAngles ToLegAngles(const LegGeometry& geometry, const Eigen::Vector3d& joints) noexcept {
  LegAngles angles{};
  angles.coxa = geometry.sigma_coxa * joints(0);
  angles.femur = LegGeometry::SigmaFemur * joints(1);
  angles.tibia = angles.femur + (LegGeometry::SigmaTibia * joints(2)) + geometry.tibia_offset;
  return angles;
}

/// Whether every joint of a triple lies within the configured travel.
[[nodiscard]] bool WithinLimits(const LegGeometry& geometry, const Eigen::Vector3d& joints) noexcept {
  return std::all_of(joints.data(), joints.data() + joints.size(),
                     [&geometry](double angle) { return angle >= geometry.joint_min && angle <= geometry.joint_max; });
}

}  // namespace

Eigen::Vector3d ForwardKinematics(const LegGeometry& geometry, const Eigen::Vector3d& joints) noexcept {
  const LegAngles angles = ToLegAngles(geometry, joints);

  const double cos_femur = std::cos(angles.femur);
  const double sin_femur = std::sin(angles.femur);
  const double cos_tibia = std::cos(angles.tibia);
  const double sin_tibia = std::sin(angles.tibia);

  // Reach measured outwards along the leg plane, and height above the hip.
  const double reach = geometry.coxa_length + (geometry.femur_length * cos_femur) + (geometry.tibia_length * cos_tibia);
  const double height = (geometry.femur_length * sin_femur) + (geometry.tibia_length * sin_tibia);

  const double cos_coxa = std::cos(angles.coxa);
  const double sin_coxa = std::sin(angles.coxa);

  // The leg plane is displaced sideways by lateral_offset, so the foot is the
  // coxa rotation applied to (reach, lateral_offset).
  return {(reach * cos_coxa) - (geometry.lateral_offset * sin_coxa),
          (reach * sin_coxa) + (geometry.lateral_offset * cos_coxa), height};
}

LegState ForwardKinematicsWithJacobian(const LegGeometry& geometry, const Eigen::Vector3d& joints) noexcept {
  LegState state{};
  state.foot = ForwardKinematics(geometry, joints);

  const LegAngles angles = ToLegAngles(geometry, joints);

  const double cos_femur = std::cos(angles.femur);
  const double sin_femur = std::sin(angles.femur);
  const double cos_tibia = std::cos(angles.tibia);
  const double sin_tibia = std::sin(angles.tibia);
  const double cos_coxa = std::cos(angles.coxa);
  const double sin_coxa = std::sin(angles.coxa);

  const double reach = geometry.coxa_length + (geometry.femur_length * cos_femur) + (geometry.tibia_length * cos_tibia);

  // d(reach)/d(joint) and d(height)/d(joint). The femur joint moves both links
  // because the tibia angle is measured from the femur; the tibia joint moves
  // only the last one.
  const double reach_d_femur =
      LegGeometry::SigmaFemur * ((-geometry.femur_length * sin_femur) - (geometry.tibia_length * sin_tibia));
  const double height_d_femur =
      LegGeometry::SigmaFemur * ((geometry.femur_length * cos_femur) + (geometry.tibia_length * cos_tibia));
  const double reach_d_tibia = LegGeometry::SigmaTibia * (-geometry.tibia_length * sin_tibia);
  const double height_d_tibia = LegGeometry::SigmaTibia * (geometry.tibia_length * cos_tibia);

  // Coxa column: rotating the whole leg plane about the hip z axis.
  state.jacobian(0, 0) = geometry.sigma_coxa * ((-reach * sin_coxa) - (geometry.lateral_offset * cos_coxa));
  state.jacobian(1, 0) = geometry.sigma_coxa * ((reach * cos_coxa) - (geometry.lateral_offset * sin_coxa));
  state.jacobian(2, 0) = 0.0;

  // Femur and tibia columns: motion stays in the leg plane, then is rotated.
  state.jacobian(0, 1) = reach_d_femur * cos_coxa;
  state.jacobian(1, 1) = reach_d_femur * sin_coxa;
  state.jacobian(2, 1) = height_d_femur;

  state.jacobian(0, 2) = reach_d_tibia * cos_coxa;
  state.jacobian(1, 2) = reach_d_tibia * sin_coxa;
  state.jacobian(2, 2) = height_d_tibia;

  return state;
}

InverseKinematicsResult InverseKinematics(const LegGeometry& geometry, const Eigen::Vector3d& target) noexcept {
  InverseKinematicsResult result{};

  // An unpopulated geometry would divide by zero below and hand back NaNs that
  // look like joint angles. Refuse instead.
  if (!geometry.IsUsable()) {
    result.status = InverseKinematicsStatus::Unreachable;
    return result;
  }

  // --- coxa ----------------------------------------------------------------
  // Forward: (x, y) = Rz(coxa) * (reach, lateral_offset). The rotation
  // preserves length, so reach follows from the horizontal distance, and the
  // coxa angle is the difference of the two bearings.
  const double horizontal_squared = (target.x() * target.x()) + (target.y() * target.y());
  const double lateral_squared = geometry.lateral_offset * geometry.lateral_offset;
  if (horizontal_squared < lateral_squared) {
    // Inside the cylinder the leg plane can never enter.
    result.status = InverseKinematicsStatus::Unreachable;
    return result;
  }

  const double reach = std::sqrt(horizontal_squared - lateral_squared);
  const double coxa_angle = std::atan2(target.y(), target.x()) - std::atan2(geometry.lateral_offset, reach);

  // --- femur and tibia -----------------------------------------------------
  // A planar two-link arm from the femur pivot to the foot.
  const double planar_reach = reach - geometry.coxa_length;
  const double planar_height = target.z();
  const double distance_squared = (planar_reach * planar_reach) + (planar_height * planar_height);
  const double distance = std::sqrt(distance_squared);

  const double femur = geometry.femur_length;
  const double tibia = geometry.tibia_length;
  if (distance > (femur + tibia) || distance < std::abs(femur - tibia)) {
    result.status = InverseKinematicsStatus::Unreachable;
    return result;
  }

  // Law of cosines for the angle between the links. Clamped because a target
  // exactly on the boundary can land a hair outside [-1, 1] in floating point,
  // and std::acos would return NaN for it.
  const double cos_interior =
      std::clamp((distance_squared - (femur * femur) - (tibia * tibia)) / (2.0 * femur * tibia), -1.0, 1.0);

  // Two elbow configurations reach any point; the negative branch is the one
  // that bends the way the rest pose does, so a solved pose never flips the
  // knee mid-stride.
  const double interior = -std::acos(cos_interior);

  const double femur_angle = std::atan2(planar_height, planar_reach) -
                             std::atan2(tibia * std::sin(interior), femur + (tibia * std::cos(interior)));

  // Undo the sign conventions to get back to joint angles. sigma_coxa is +/-1
  // and IsUsable() has already established that, so none of these divide by
  // zero.
  result.q = {WrapToPi(coxa_angle / geometry.sigma_coxa), WrapToPi(femur_angle / LegGeometry::SigmaFemur),
              WrapToPi((interior - geometry.tibia_offset) / LegGeometry::SigmaTibia)};

  // The pose is geometrically right but the servo may not be able to hold it.
  // Report that separately, and keep the angles so a caller can say how far out
  // of range the request was.
  result.status =
      WithinLimits(geometry, result.q) ? InverseKinematicsStatus::Success : InverseKinematicsStatus::LimitViolation;
  return result;
}

}  // namespace hexapod::model
