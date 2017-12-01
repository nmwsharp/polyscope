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
  
  // Extrinsic transform
  glm::mat4x4 E;
  
  // Intrinsics
  // glm::vec2 imageCenter;   // measured in distance, NOT pixels
  glm::vec2 focalLengths;  // measured in distance, NOT pixels

  // Get various derived quantities
  glm::vec3 getT();
  glm::mat3x3 getR();
  glm::vec3 getPosition();
  glm::vec3 getLookDir();
  glm::vec3 getUpDir();
  glm::vec3 getRightDir();

};

// Print GLM matrices in nice ways
void prettyPrint(glm::mat4x4 M);

}  // namespace polyscope