#pragma once

#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"

namespace polyscope {

// Utility class to track the parameters of a camera
// Note that these DO NOT include any particular image discretization (which
// would be measured in pixels)
class CameraParameters {
 public:
  CameraParameters();
  
  // Extrinsics
  // glm::vec3 T;    // position of world orgin, in camera coords
  //                 // (NOT world location)
  // glm::mat3x3 R;  // rotation from world coords to camera coords
  glm::mat4x4 E; // extrinsic transform
  
  // Intrinsics
  // glm::vec2 imageCenter;   // measured in distance, NOT pixels
  glm::vec2 focalLengths;  // measured in distance, NOT pixels

};

}  // namespace polyscope