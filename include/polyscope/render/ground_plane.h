// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/view.h"

#include <memory>

namespace polyscope {
namespace render {

// Forward declare necessary types
// TODO get this out of the engine so this isn't necessary
class ShaderProgram;
class TextureBuffer;
class FrameBuffer;
class RenderBuffer;

// There should probably only ever be one GroundPlane object, managed by the render::Engine.
class GroundPlane {
public:
  GroundPlane(){};

  void draw();
  void buildGui();

  // How far should the ground plane be from the bottom of the scene? Measured as a multiple of the vertical bounding
  // box of the scene.
  float groundPlaneHeightFactor = 0;

private:
  std::shared_ptr<render::ShaderProgram> groundPlaneProgram;
  std::shared_ptr<render::TextureBuffer> mirroredSceneColorTexture;
  std::shared_ptr<render::RenderBuffer> mirroredSceneDepth;
  std::shared_ptr<render::FrameBuffer> mirroredSceneFrameBuffer;

  // alternating blurring
  // result starts and ends in the first buffer
  std::array<std::shared_ptr<render::TextureBuffer>, 2> blurColorTextures;
  std::array<std::shared_ptr<render::FrameBuffer>, 2> blurFrameBuffers;
  std::shared_ptr<render::ShaderProgram> blurProgram, copyTexProgram;
  void blurIteration();

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
