// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/camera_parameters.h"

#include <cmath>
#include <iostream>

#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace polyscope {

// == Intrinsics

CameraIntrinsics::CameraIntrinsics() : fovVerticalDegrees(-1), aspectRatioWidthOverHeight(-1), isValidFlag(true) {}

CameraIntrinsics::CameraIntrinsics(const float& fovVerticalDegrees_, const float& aspectRatioWidthOverHeight_)
    : fovVerticalDegrees(fovVerticalDegrees_), aspectRatioWidthOverHeight(aspectRatioWidthOverHeight_),
      isValidFlag(true) {}

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

CameraIntrinsics CameraIntrinsics::createInvalid() { return CameraIntrinsics(); }
bool CameraIntrinsics::isValid() const { return isValidFlag; }

float CameraIntrinsics::getFoVVerticalDegrees() const { return fovVerticalDegrees; }
float CameraIntrinsics::getAspectRatioWidthOverHeight() const { return aspectRatioWidthOverHeight; }


// == Extrinsics

CameraExtrinsics::CameraExtrinsics() : E(-777.f), isValidFlag(false) {}
CameraExtrinsics::CameraExtrinsics(const glm::mat4& E_) : E(E_), isValidFlag(true) {}

CameraExtrinsics CameraExtrinsics::fromMatrix(const glm::mat4& E) { return CameraExtrinsics(E); }

CameraExtrinsics CameraExtrinsics::createInvalid() { return CameraExtrinsics(); }
bool CameraExtrinsics::isValid() const { return isValidFlag; }

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

std::vector<glm::vec3> CameraParameters::generateCameraRays(size_t dimX, size_t dimY, ImageOrigin origin) const {

  // TODO this has not been tested for the case where dimX/dimY do not match the aspectRatio for the cameraParams

  // prep values
  glm::mat4x4 viewMat = getViewMat();
  glm::mat4 projMat =
      glm::infinitePerspective(glm::radians(getFoVVerticalDegrees()), getAspectRatioWidthOverHeight(), 1.f);
  glm::vec4 viewport = {0., 0., dimX, dimY};
  glm::vec3 rootPos = getPosition();
  size_t nPix = dimX * dimY;

  // allocate output
  std::vector<glm::vec3> result(dimX * dimY);

  // populate the rays
  for (size_t iY = 0; iY < dimY; iY++) {
    for (size_t iX = 0; iX < dimX; iX++) {

      // populate the rays
      // (this arithmetic could certainly be optimized)

      glm::vec3 screenPos3;
      switch (origin) {
      case ImageOrigin::UpperLeft:
        screenPos3 = glm::vec3{iX, dimY - iY, 0.};
        break;
      case ImageOrigin::LowerLeft:
        screenPos3 = glm::vec3{iX, iY, 0.};
        break;
      }

      glm::vec3 worldPos = glm::unProject(screenPos3, viewMat, projMat, viewport);
      glm::vec3 worldRayDir = glm::normalize(worldPos - rootPos);

      size_t ind = iY * dimX + iX;
      result[ind] = worldRayDir;
    }
  }

  return result;
}

// create/test 'invalid' params
CameraParameters CameraParameters::createInvalid() {
  return CameraParameters(CameraIntrinsics::createInvalid(), CameraExtrinsics::createInvalid());
}
bool CameraParameters::isValid() const { return intrinsics.isValid() && extrinsics.isValid(); }

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
