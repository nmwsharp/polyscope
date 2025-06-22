// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

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
  GroundPlane() {};

  // Render the ground plane.
  // isRedraw allows for an optimization: for rendering modes where the ground gets drawn many times per-frame, we only
  // need to generate expensive reflections (etc) once. Setting isRedraw=true skips generating this data.
  void draw(bool isRedraw = false);

  void buildGui();
  void prepare(); // does any and all setup work / allocations / etc, called automatically when drawing after a change


  // == Appearance Parameters
  // These all now live in polyscope::options

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

  // track if the ground plane has been prepared, and if so in what style
  bool groundPlanePrepared = false;
  GroundPlaneMode groundPlanePreparedMode = GroundPlaneMode::None;
  // which direction the ground plane faces
  view::UpDir groundPlaneViewCached = view::UpDir::XUp; // not actually valid, must populate first time
};

} // namespace render


} // namespace polyscope
