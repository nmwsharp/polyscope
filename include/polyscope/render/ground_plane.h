// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/view.h"

namespace polyscope {
namespace render {

// Forward declare necessary types
class ShaderProgram;
class TextureBuffer;
class FrameBuffer;

// There should probably only ever be one GroundPlane object, managed by the render::Engine.
class GroundPlane {
public:
  GroundPlane() {};

  void draw();
  void buildGui();

  // How far should the ground plane be from the bottom of the scene? Measured as a multiple of the vertical bounding
  // box of the scene.
  float groundPlaneHeightFactor = 0;

  // TODO pbr settings
  float pbrRoughness = 0.35;
  float pbrMetallic = 0.0;
  float pbrF0 = 0.05;

private:
  std::shared_ptr<render::ShaderProgram> groundPlaneProgram = nullptr;
  std::shared_ptr<render::TextureBuffer> mirroredSceneColorTexture = nullptr;
  std::shared_ptr<render::FrameBuffer> mirroredSceneFrameBuffer = nullptr;

  void prepareGroundPlane();
  void populateGroundPlaneGeometry();
  bool groundPlanePrepared = false;
  // which direction the ground plane faces
  view::UpDir groundPlaneViewCached = view::UpDir::XUp; // not actually valid, must populate first time
};

} // namespace render

// put this in the options namespace to get nicer syntax on the option
namespace options {
extern bool groundPlaneEnabled; // true by default
}

} // namespace polyscope
