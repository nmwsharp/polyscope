// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <tuple>

#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/mat3x3.hpp"
#include "glm/vec3.hpp"

#include "polyscope/standardize_data_array.h"
#include "polyscope/types.h"

namespace polyscope {

// Utility class to track the parameters of a camera
// Note that these DO NOT include any particular image discretization (which
// would be measured in pixels)

// Polyscope's cameras use openGL camera conventions. We construct an intrinsic matrix that maps to NDC coordinates on
// [-1,1]^3 after homogenous division, with the camera looking down the -Z axis

class CameraIntrinsics {
public:
  // Constructor (prefer the named constructors below)
  CameraIntrinsics();
  CameraIntrinsics(const float& FoVVerticalDegrees, const float& aspectRatioWidthOverHeight);

  // Named constructors from various combinations of values
  static CameraIntrinsics fromFoVDegVerticalAndAspect(const float& fovVertDeg, const float& aspectRatioWidthOverHeight);
  static CameraIntrinsics fromFoVDegHorizontalAndAspect(const float& fovHorzDeg,
                                                        const float& aspectRatioWidthOverHeight);
  static CameraIntrinsics fromFoVDegHorizontalAndVertical(const float& fovHorzDeg, const float& forV);

  // create/test 'invalid' params
  static CameraIntrinsics createInvalid();
  bool isValid() const;

  // == Intrinsic getters
  float getFoVVerticalDegrees() const;
  float getAspectRatioWidthOverHeight() const;

private:
  float fovVerticalDegrees;         // the angle, in degrees from the top to bottom of the viewing frustum
  float aspectRatioWidthOverHeight; // the ratio of the viewing frustum width / height
  bool isValidFlag;
};

class CameraExtrinsics {
public:
  // Constructor (prefer the named constructors below)
  CameraExtrinsics();
  CameraExtrinsics(const glm::mat4& E);

  // Named constructors from various combinations of values
  template <class T1, class T2, class T3>
  static CameraExtrinsics fromVectors(const T1& root, const T2& lookDir, const T3& upDir);
  static CameraExtrinsics fromMatrix(const glm::mat4& E);

  // create/test 'invalid' params
  static CameraExtrinsics createInvalid();
  bool isValid() const;

  // == Extrinsic getters
  glm::vec3 getT() const;
  glm::mat3x3 getR() const;
  glm::mat4x4 getViewMat() const; // same as getE
  glm::mat4x4 getE() const;
  glm::vec3 getPosition() const;
  glm::vec3 getLookDir() const;
  glm::vec3 getUpDir() const;
  glm::vec3 getRightDir() const;
  std::tuple<glm::vec3, glm::vec3, glm::vec3> getCameraFrame() const;

private:
  glm::mat4x4 E; // E * p maps p to eye space, where the camera is at the origin and looks down the -Z axis,
  bool isValidFlag;
};

class CameraParameters {
public:
  CameraParameters();
  CameraParameters(const CameraIntrinsics& intrinsics, const CameraExtrinsics& extrinsics);

  // create/test 'invalid' params
  static CameraParameters createInvalid();
  bool isValid() const;

  // The intrinsic & extrinsics parameters that define the camera
  CameraIntrinsics intrinsics;
  CameraExtrinsics extrinsics;

  // Fill a buffer with rays corresponding to an image with the given resolution
  // The result is a dimX*dimY-length buffer.
  // Ray origins are all implicitly given by this->getPosition()
  std::vector<glm::vec3> generateCameraRays(size_t dimX, size_t dimY,
                                            ImageOrigin origin = ImageOrigin::UpperLeft) const;

  // (these getters are just forwarded from the intrinsics/extrinsics, for convenience)

  // == Extrinsic getters
  glm::vec3 getT() const;
  glm::mat3x3 getR() const;
  glm::mat4x4 getViewMat() const; // same as getE
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
};

} // namespace polyscope

#include "polyscope/camera_parameters.ipp"
