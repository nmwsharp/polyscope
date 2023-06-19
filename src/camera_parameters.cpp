// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/camera_parameters.h"

#include <cmath>
#include <iostream>

#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace polyscope {

// == Intrinsics

CameraIntrinsics::CameraIntrinsics() {}
CameraIntrinsics::CameraIntrinsics(const float& fovVerticalDegrees_, const float& aspectRatioWidthOverHeight_)
    : fovVerticalDegrees(fovVerticalDegrees_), aspectRatioWidthOverHeight(aspectRatioWidthOverHeight_){};

CameraIntrinsics CameraIntrinsics::fromFoVDegVerticalAndAspect(const float& fovVertDeg,
                                                               const float& aspectRatioWidthOverHeight) {
  return CameraIntrinsics(fovVertDeg, aspectRatioWidthOverHeight);
}
CameraIntrinsics CameraIntrinsics::fromFoVDegHorizontalAndAspect(const float& fovHorzDeg,
                                                                 const float& aspectRatioWidthOverHeight) {
  float fovVertDeg =
      glm::degrees(2.f * std::atan(std::tan(0.5f * glm::radians(fovHorzDeg)) / aspectRatioWidthOverHeight));
  return CameraIntrinsics(fovVertDeg, aspectRatioWidthOverHeight);
}
CameraIntrinsics CameraIntrinsics::fromFoVDegHorizontalAndVertical(const float& fovHorzDeg, const float& fovVertDeg) {
  float aspectRatioWidthOverHeight =
      std::tan(0.5f * glm::radians(fovHorzDeg)) / std::tan(0.5f * glm::radians(fovVertDeg));
  return CameraIntrinsics(fovVertDeg, aspectRatioWidthOverHeight);
}

float CameraIntrinsics::getFoVVerticalDegrees() const { return fovVerticalDegrees; }
float CameraIntrinsics::getAspectRatioWidthOverHeight() const { return aspectRatioWidthOverHeight; }


// == Extrinsics

CameraExtrinsics::CameraExtrinsics() {}
CameraExtrinsics::CameraExtrinsics(const glm::mat4& E_) : E(E_) {}

CameraExtrinsics CameraExtrinsics::fromMatrix(const glm::mat4& E) { return CameraExtrinsics(E); }

glm::mat4x4 CameraExtrinsics::getViewMat() const { return E; }
glm::mat4x4 CameraExtrinsics::getE() const { return E; }
glm::vec3 CameraExtrinsics::getT() const { return glm::vec3(E[3][0], E[3][1], E[3][2]); }
glm::mat3x3 CameraExtrinsics::getR() const {
  glm::mat3x3 R;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      R[i][j] = E[i][j];
    }
  }
  return R;
}

// TODO these are duplicated in view.h

glm::vec3 CameraExtrinsics::getPosition() const { return -transpose(getR()) * getT(); }
glm::vec3 CameraExtrinsics::getLookDir() const { return normalize(transpose(getR()) * glm::vec3(0.0, 0.0, -1.0)); }
glm::vec3 CameraExtrinsics::getUpDir() const { return normalize(transpose(getR()) * glm::vec3(0.0, 1.0, 0.0)); }
glm::vec3 CameraExtrinsics::getRightDir() const { return normalize(transpose(getR()) * glm::vec3(1.0, 0.0, 0.0)); }
std::tuple<glm::vec3, glm::vec3, glm::vec3> CameraExtrinsics::getCameraFrame() const {

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

// == Camera Parameters
// (intrinsics + extrinsics)

CameraParameters::CameraParameters() {}

CameraParameters::CameraParameters(const CameraIntrinsics& intrinsics_, const CameraExtrinsics& extrinsics_)
    : intrinsics(intrinsics_), extrinsics(extrinsics_) {}


// == Forwarding getters for the camera class

glm::vec3 CameraParameters::getT() const { return extrinsics.getT(); }
glm::mat3x3 CameraParameters::getR() const { return extrinsics.getR(); }
glm::mat4x4 CameraParameters::getViewMat() const { return extrinsics.getViewMat(); }
glm::mat4x4 CameraParameters::getE() const { return extrinsics.getE(); }
glm::vec3 CameraParameters::getPosition() const { return extrinsics.getPosition(); }
glm::vec3 CameraParameters::getLookDir() const { return extrinsics.getLookDir(); }
glm::vec3 CameraParameters::getUpDir() const { return extrinsics.getUpDir(); }
glm::vec3 CameraParameters::getRightDir() const { return extrinsics.getRightDir(); }
std::tuple<glm::vec3, glm::vec3, glm::vec3> CameraParameters::getCameraFrame() const {
  return extrinsics.getCameraFrame();
}
float CameraParameters::getFoVVerticalDegrees() const { return intrinsics.getFoVVerticalDegrees(); }
float CameraParameters::getAspectRatioWidthOverHeight() const { return intrinsics.getAspectRatioWidthOverHeight(); }


} // namespace polyscope
