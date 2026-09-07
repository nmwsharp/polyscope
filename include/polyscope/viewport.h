// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <array>
#include <memory>
#include <string>

#include "polyscope/types.h"
#include "polyscope/view.h"

namespace polyscope {

// Forward declarations
namespace render {
class FrameBuffer;
class TextureBuffer;
class ShaderProgram;
} // namespace render

// A snapshot of the global view state, used for push/pop when rendering viewports
struct ViewStateSnapshot {
  glm::mat4x4 viewMat;
  float fov;
  glm::vec3 viewCenter;
  NavigateStyle navigateStyle;
  UpDir upDir;
  FrontDir frontDir;
  ProjectionMode projectionMode;
  float nearClip, farClip;
  float moveScale;
  ViewRelativeMode viewRelativeMode;
  std::array<float, 4> bgColor;
  int bufferWidth, bufferHeight;
  int windowWidth, windowHeight;

  // Flight state
  bool midflight;
  float flightStartTime, flightEndTime;
  glm::dualquat flightTargetViewR, flightInitialViewR;
  glm::vec3 flightTargetViewT, flightInitialViewT;
  float flightTargetFov, flightInitialFov;
};

// Save/restore the current global view state
ViewStateSnapshot saveViewState();
void restoreViewState(const ViewStateSnapshot& snapshot);


class Viewport {
public:
  Viewport(std::string name, int gridRow, int gridCol);
  ~Viewport();

  // === Identity
  std::string name;
  int gridRow, gridCol;

  // === Camera state (independent per viewport)
  glm::mat4x4 viewMat;
  float fov;
  glm::vec3 viewCenter;
  NavigateStyle navigateStyle;
  UpDir upDir;
  FrontDir frontDir;
  ProjectionMode projectionMode;
  float nearClip, farClip;
  float moveScale;
  ViewRelativeMode viewRelativeMode;
  std::array<float, 4> bgColor;

  // Flight state
  bool midflight;
  float flightStartTime, flightEndTime;
  glm::dualquat flightTargetViewR, flightInitialViewR;
  glm::vec3 flightTargetViewT, flightInitialViewT;
  float flightTargetFov, flightInitialFov;

  // === Pixel region (computed from grid layout + window size)
  int pixelX, pixelY;          // lower-left corner in buffer coords
  int pixelWidth, pixelHeight; // size in buffer pixels
  int windowX, windowY;        // lower-left corner in window coords
  int windowW, windowH;        // size in window pixels

  // === Framebuffers (per-viewport)
  std::shared_ptr<render::FrameBuffer> sceneBuffer;
  std::shared_ptr<render::FrameBuffer> sceneBufferFinal;
  std::shared_ptr<render::FrameBuffer> sceneDepthMinFrame;
  std::shared_ptr<render::TextureBuffer> sceneColor, sceneColorFinal, sceneDepth, sceneDepthMin;
  std::shared_ptr<render::ShaderProgram> compositePeel;

  // === Methods

  // Push this viewport's camera state into the global view:: variables
  void pushViewState();

  // Store the current global view state back into this viewport
  void pullViewState();

  // Recompute pixel dimensions from grid layout and window size
  void updateLayout(int totalBufferW, int totalBufferH, int totalWindowW, int totalWindowH, int gridRows, int gridCols);

  // Create or resize per-viewport framebuffers
  void ensureBuffersAllocated();
  void resizeBuffers();

  // Reset camera to the home view for this viewport
  void resetCameraToHomeView();

  // Set navigate style with proper camera adjustment (use instead of setting navigateStyle directly)
  void setNavigateStyle(NavigateStyle newStyle, bool animateFlight = false);

  // Set up direction with proper camera adjustment (use instead of setting upDir directly)
  void setUpDir(UpDir newUpDir, bool animateFlight = false);

  // Check if screen coordinates (in window space) fall within this viewport
  bool containsScreenCoords(float x, float y) const;
};

} // namespace polyscope
