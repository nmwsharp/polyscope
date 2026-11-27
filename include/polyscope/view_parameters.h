// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <array>
#include <vector>

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
  // posX/posY is the top-left corner (y=0 at the top, like screen coords).
  float viewportRelativePosX;
  float viewportRelativePosY;
  float viewportRelativeWidth;
  float viewportRelativeHeight;

  // Computed viewport geometry derived from the relative values above.
  // See note in view.h for the distinction of bufferIndices vs screenCoords.
  int viewportPosX;           // top-left corner of the viewport in buffer inds (integer pixel inds)
  int viewportPosY;           // (y=0 at the top, like buffer inds everywhere else. OpenGL calls flip this as needed)
  int viewportBufferWidth;    // viewport dimensions in buffer inds (integer buffer pixels)
  int viewportBufferHeight;   // viewport dimensions in buffer inds (integer buffer pixels)
  float viewportWindowWidth;  // viewport dimensions in screenCoords (floats, as in ImGui etc)
  float viewportWindowHeight; // viewport dimensions in screenCoords (floats, as in ImGui etc)
  bool overlapsEarlierViewport; // true if this viewport overlaps any lower-indexed viewport
};

// A group of mutually non-overlapping viewports that can be rendered together in one pass, along with their
// clamped union bounding box in top-down buffer coordinates.
struct ViewportLayer {
  std::vector<int> viewportIndices;
  int minX = 0;
  int minY = 0;
  int maxX = 0;
  int maxY = 0;
  bool hasArea() const { return minX < maxX && minY < maxY; }
};

} // namespace view
} // namespace polyscope
