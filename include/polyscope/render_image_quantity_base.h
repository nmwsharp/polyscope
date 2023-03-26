// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/polyscope.h"

#include "polyscope/floating_quantity.h"

#include <vector>

/*
 * Base class for RenderImage classes, which are render-like data of onscreen geometry (buffers of depth, normals, etc)
 * which has been generated from some out-of-Polyscope process, and is to be rendered to the screen.
 *
 */

namespace polyscope {

class RenderImageQuantityBase : public FloatingQuantity {

public:
  RenderImageQuantityBase(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                          const std::vector<float>& depthData, const std::vector<glm::vec3>& normalData,
                          ImageOrigin imageOrigin);

  // virtual void draw() override;
  // virtual void drawDelayed() override;

  virtual void refresh() override;

  size_t nPix();

  void updateGeometryBuffers(const std::vector<float>& newDepthData, const std::vector<glm::vec3>& newNormalData);

  // == Setters and getters

  // Material
  RenderImageQuantityBase* setMaterial(std::string name);
  std::string getMaterial();

  // Transparency
  RenderImageQuantityBase* setTransparency(float newVal);
  float getTransparency();


protected:
  const size_t dimX, dimY;
  ImageOrigin imageOrigin;

  // Store the raw data
  std::vector<float> depthData;
  std::vector<glm::vec3> normalData;

  // === Visualization parameters
  PersistentValue<std::string> material;
  PersistentValue<float> transparency;

  std::shared_ptr<render::TextureBuffer> textureDepth, textureNormal;

  // Helpers
  void prepareGeometryBuffers();
  void addOptionsPopupEntries();
};


} // namespace polyscope
