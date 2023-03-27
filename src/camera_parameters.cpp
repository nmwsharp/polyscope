// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/camera_parameters.h"

#include <cstdio>
#include <iostream>

#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace polyscope {

// CameraParameters::CameraParameters()
//     : T(0.0), R(glm::mat3x3(1.0)), focalLengths(1.0) {}
// CameraParameters::CameraParameters() : E(1.0), focalLengths(1.0) {}
CameraParameters::CameraParameters() : E(1.0), fovVerticalDegrees(60.) {}

CameraParameters::CameraParameters(glm::mat4 E_, float fovVertDeg_, float aspectRatioWidthOverHeight_)
    : E(E_), fovVerticalDegrees(fovVertDeg_), aspectRatioWidthOverHeight(aspectRatioWidthOverHeight_) {}

CameraParameters::CameraParameters(glm::vec3 root, glm::vec3 lookDir, glm::vec3 upDir, float fovVertDeg_,
                                   float aspectRatioWidthOverHeight_)
    : fovVerticalDegrees(fovVertDeg_), aspectRatioWidthOverHeight(aspectRatioWidthOverHeight_) {
  lookDir = glm::normalize(lookDir);
  upDir = glm::normalize(upDir);
  E = glm::lookAt(root, root + lookDir, upDir);
}

glm::mat4x4 CameraParameters::getE() const { return E; }

glm::vec3 CameraParameters::getT() const { return glm::vec3(E[3][0], E[3][1], E[3][2]); }

glm::mat3x3 CameraParameters::getR() const {
  glm::mat3x3 R;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      R[i][j] = E[i][j];
    }
  }
  return R;
}

// TODO these are duplicated in view.h

glm::vec3 CameraParameters::getPosition() const { return -transpose(getR()) * getT(); }

glm::vec3 CameraParameters::getLookDir() const { return normalize(transpose(getR()) * glm::vec3(0.0, 0.0, -1.0)); }

glm::vec3 CameraParameters::getUpDir() const { return normalize(transpose(getR()) * glm::vec3(0.0, 1.0, 0.0)); }

glm::vec3 CameraParameters::getRightDir() const { return normalize(transpose(getR()) * glm::vec3(1.0, 0.0, 0.0)); }

std::tuple<glm::vec3, glm::vec3, glm::vec3> CameraParameters::getCameraFrame() const {

  glm::mat3x3 R;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      R[i][j] = E[i][j];
    }
  }
  glm::mat3x3 Rt = glm::transpose(R);

  glm::vec3 lookDir = Rt * glm::vec3(0.0, 0.0, -1.0);
  glm::vec3 upDir = Rt * glm::vec3(0.0, 1.0, 0.0);
  glm::vec3 rightDir = Rt * glm::vec3(1.0, 0.0, 0.0);

  return std::tuple<glm::vec3, glm::vec3, glm::vec3>{lookDir, upDir, rightDir};
}

float CameraParameters::getFoVVerticalDegrees() const { return fovVerticalDegrees; }
float CameraParameters::getAspectRatioWidthOverHeight() const { return aspectRatioWidthOverHeight; }

} // namespace polyscope
