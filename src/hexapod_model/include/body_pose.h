#ifndef HEXAPOD_MODEL_BODY_POSE_H_
#define HEXAPOD_MODEL_BODY_POSE_H_

#include <Eigen/Core>

namespace hexapod::model {

class BodyPose {
 public:
 private:
  Eigen::Vector3d position_{Eigen::Vector3d::Zero()};
  Eigen::Quaterniond orientation_{Eigen::Quaterniond::Identity()};
};

}  // namespace hexapod::model

#endif  // HEXAPOD_MODEL_BODY_POSE_H_
