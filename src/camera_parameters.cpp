#include "polyscope/camera_parameters.h"

namespace polyscope {

CameraParameters::CameraParameters()
    : T(0.0), R(glm::mat3x3(1.0)), focalLengths(1.0) {}

}  // namespace polyscope