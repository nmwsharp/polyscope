// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render_image_quantity_base.h"
#include "polyscope/types.h"

#include <vector>

namespace polyscope {

class ColorRenderImageQuantity : public RenderImageQuantityBase {

public:
  ColorRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                           const std::vector<float>& depthData, const std::vector<glm::vec3>& normalData,
                           const std::vector<glm::vec3>& colorData, ImageOrigin imageOrigin);

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;
  virtual ColorRenderImageQuantity* setEnabled(bool newEnabled) override;

  virtual std::string niceName() override;

  // == Setters and getters


protected:
  // === Visualization parameters

  // Store the raw data
  std::vector<glm::vec3> colorData;

  // === Render data
  std::shared_ptr<render::TextureBuffer> textureColor;
  std::shared_ptr<render::ShaderProgram> program;

  // === Helpers
  void prepare();
};


} // namespace polyscope
