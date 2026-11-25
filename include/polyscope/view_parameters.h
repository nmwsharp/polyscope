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
  ProjectionMode projectionMode;

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

  // Viewport position and size as fractions of the full window, in [0,1].
  // posX/posY is the bottom-left corner (y=0 at bottom, i.e. openGL convention).
  float viewportRelativePosX;
  float viewportRelativePosY;
  float viewportRelativeWidth;
  float viewportRelativeHeight;

  // Computed viewport geometry derived from the relative values above.
  // See note in view.h for the distinction of bufferIndices vs screenCoords.
  int viewportPosX;         // bottom-left corner of the viewport in buffer inds (integer pixel inds)
  int viewportPosY;         // bottom-left corner of the viewport in buffer inds (integer pixel inds, y-from-bottom)
  int viewportBufferWidth;  // viewport dimensions in buffer inds (integer buffer pixels)
  int viewportBufferHeight; // viewport dimensions in buffer inds (integer buffer pixels)
  int viewportWindowWidth;  // viewport dimensions in screenCoords (floats, as in ImGui etc)
  int viewportWindowHeight; // viewport dimensions in screenCoords (floats, as in ImGui etc)
};

} // namespace view
} // namespace polyscope
