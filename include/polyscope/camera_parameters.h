// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "glm/mat3x3.hpp"
#include "glm/vec3.hpp"

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
  // glm::vec2 focalLengths;  // measured in distance, NOT pixels
  float fov; // in degrees

  // Get various derived quantities
  glm::vec3 getT() const;
  glm::mat3x3 getR() const;
  glm::vec3 getPosition() const;
  glm::vec3 getLookDir() const;
  glm::vec3 getUpDir() const;
  glm::vec3 getRightDir() const;
};

// Print GLM matrices in nice ways
void prettyPrint(glm::mat4x4 M);

} // namespace polyscope