// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <array>

#include "polyscope/types.h"

#include "glm/gtx/dual_quaternion.hpp"
#include "glm/gtx/norm.hpp" // necessary for dual_quaternion below
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

namespace polyscope {
namespace view {

struct ViewParameters {

  NavigateStyle style;
  UpDir upDir;
  FrontDir frontDir;
  float moveScale;
  ViewRelativeMode viewRelativeMode;
  float nearClip;
  float farClip;
  std::array<float, 4> bgColor;

  // Current view camera parameters
  // TODO deprecate these one day, and just use a CameraParameters member instead. But this would break existing code,
  // so for now we leave these as-is and wrap inputs/outputs to a CameraParameters
  glm::mat4x4 viewMat;
  float fov;            // in the y direction
  glm::vec3 viewCenter; // center about which view transformations are performed
  // NOTE: ProjectionMode projectionMode; _should_ be an option here as well, but we some rendering functionality
  // requires different shaders based on mode, so it is stuck as a global setting for now.

  // "Flying" view members
  bool midflight;
  float flightStartTime;
  float flightEndTime;
  glm::dualquat flightTargetViewR;
  glm::dualquat flightInitialViewR;
  glm::vec3 flightTargetViewT;
  glm::vec3 flightInitialViewT;
  float flightTargetFov;
  float flightInitialFov;

  // Subview position and size within the full screen buffer (in relative [0,1] coords)
  float subviewRelativePosX;
  float subviewRelativePosY;
  float subviewRelativeWidth;
  float subviewRelativeHeight;

  // Computed subview values
  int subviewPosX; // in buffer units
  int subviewPosY; // in buffer units
  int subviewBufferWidth;
  int subviewBufferHeight;
  int subviewWindowWidth;
  int subviewWindowHeight;
};

} // namespace view
} // namespace polyscope
