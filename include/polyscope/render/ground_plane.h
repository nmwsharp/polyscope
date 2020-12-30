// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/types.h"
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

// Note: behavior is dictated by the global options::groundPlaneMode setting

// There should probably only ever be one GroundPlane object, managed by the render::Engine.
class GroundPlane {
public:
  GroundPlane(){};

  // Render the ground plane.
  // isRedraw allows for an optimization: for rendering modes where the ground gets drawn many times per-frame, we only
  // need to generate expensive reflections (etc) once. Setting isRedraw=true skips generating this data.
  void draw(bool isRedraw = false);

  void buildGui();
  void prepare(); // does any and all setup work / allocations / etc. Should be called whenever the mode is changed.


  // == Appearance Parameters

  // These all now live in polyscope::options

  // How far should the ground plane be from the bottom of the scene? Measured as a multiple of the vertical bounding
  // box of the scene.
  // float groundPlaneHeightFactor = 0;

  // int blurIters = 4; // how much blurring to do for
  // float shadowDarkness = 0.4; // how dark to make the shadows

private:
  // note: these buffers/programs are only optionally populated based on the mode

  std::shared_ptr<render::ShaderProgram> groundPlaneProgram;
  std::shared_ptr<render::TextureBuffer> sceneAltColorTexture;
  std::shared_ptr<render::TextureBuffer> sceneAltDepthTexture;
  std::shared_ptr<render::FrameBuffer> sceneAltFrameBuffer;

  // alternating blurring
  // result starts and ends in the first buffer
  std::array<std::shared_ptr<render::TextureBuffer>, 2> blurColorTextures;
  std::array<std::shared_ptr<render::FrameBuffer>, 2> blurFrameBuffers;
  std::shared_ptr<render::ShaderProgram> blurProgram, copyTexProgram;

  void populateGroundPlaneGeometry();
  bool groundPlanePrepared = false;
  // which direction the ground plane faces
  view::UpDir groundPlaneViewCached = view::UpDir::XUp; // not actually valid, must populate first time
};

} // namespace render


} // namespace polyscope
