// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <tuple>

#include "glm/mat3x3.hpp"
#include "glm/vec3.hpp"

namespace polyscope {

// Utility class to track the parameters of a camera
// Note that these DO NOT include any particular image discretization (which
// would be measured in pixels)
class CameraParameters {
  //
  // Polyscope's cameras use openGL camera conventions. We construct an intrinsic matrix that maps to NDC
  // coordinates on [-1,1]^3 after homogenous division, with the camer
  //
public:
  CameraParameters();
  CameraParameters(glm::mat4 E, float fovVertDeg, float aspectRatioWidthOverHeight);
  CameraParameters(glm::vec3 root, glm::vec3 lookDir, glm::vec3 upDir, float fovVertDeg,
                   float aspectRatioWidthOverHeight);


  // == Extrinsic getters
  glm::vec3 getT() const;
  glm::mat3x3 getR() const;
  glm::mat4x4 getE() const;
  glm::vec3 getPosition() const;
  glm::vec3 getLookDir() const;
  glm::vec3 getUpDir() const;
  glm::vec3 getRightDir() const;
  std::tuple<glm::vec3, glm::vec3, glm::vec3> getCameraFrame() const; // <look, up, right>

  // == Intrinsic getters
  float getFoVVerticalDegrees() const;
  float getAspectRatioWidthOverHeight() const;

private:
  // Extrinsic transform
  glm::mat4x4 E; // E * p maps p to eye space, where the camera is at the origin and looks down the -Z axis,

  // Intrinsics
  float fovVerticalDegrees;         // the angle, in degrees from the top to bottom of the viewing frustum
  float aspectRatioWidthOverHeight; // the ratio of the viewing frustum width / height
};

} // namespace polyscope
