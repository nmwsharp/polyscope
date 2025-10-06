// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/polyscope.h"

#include "polyscope/floating_quantity.h"
#include "polyscope/fullscreen_artist.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/structure.h"

#include <vector>

/*
 * Base class for RenderImage classes, which are render-like data of onscreen geometry (buffers of depth, normals, etc)
 * which has been generated from some out-of-Polyscope process, and is to be rendered to the screen.
 *
 */

namespace polyscope {

class RenderImageQuantityBase : public FloatingQuantity, public FullscreenArtist {

public:
  RenderImageQuantityBase(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                          const std::vector<float>& depthData, const std::vector<glm::vec3>& normalData,
                          ImageOrigin imageOrigin);

  virtual void drawPickDelayed() override;

  virtual void refresh() override;

  size_t nPix();

  render::ManagedBuffer<float> depths;
  render::ManagedBuffer<glm::vec3> normals;

  void updateBaseBuffers(const std::vector<float>& newDepthData, const std::vector<glm::vec3>& newNormalData);

  virtual void disableFullscreenDrawing() override;

  // == Setters and getters

  virtual RenderImageQuantityBase* setEnabled(bool newEnabled) override;

  // Material
  RenderImageQuantityBase* setMaterial(std::string name);
  std::string getMaterial();

  // Transparency
  RenderImageQuantityBase* setTransparency(float newVal);
  float getTransparency();

  // Fullscreen compositing
  // This controls whether multiple of these been shown fullscreen at the same time, vs do they dominate each other and
  // allow only one to be enabled. By default, false, only one can be enabled.
  RenderImageQuantityBase* setAllowFullscreenCompositing(bool newVal);
  bool getAllowFullscreenCompositing();


protected:
  const size_t dimX, dimY;
  const bool hasNormals;
  ImageOrigin imageOrigin;

  // Store the raw data
  std::vector<float> depthsData;
  std::vector<glm::vec3> normalsData;

  // === Visualization parameters
  PersistentValue<std::string> material;
  PersistentValue<float> transparency;
  PersistentValue<bool> allowFullscreenCompositing;

  // Picking is the same for all
  std::shared_ptr<render::ShaderProgram> pickProgram;
  glm::vec3 pickColor;

  // Helpers
  void prepareGeometryBuffers();
  void addOptionsPopupEntries();
  void preparePick();
  void setRenderImageUniforms(render::ShaderProgram& program, bool withTonemap = false);
};


} // namespace polyscope
