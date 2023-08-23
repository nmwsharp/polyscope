// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render_image_quantity_base.h"
#include "polyscope/types.h"

#include <vector>

namespace polyscope {

class RawColorRenderImageQuantity : public RenderImageQuantityBase {

public:
  RawColorRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                              const std::vector<float>& depthData, const std::vector<glm::vec3>& colorsData,
                              ImageOrigin imageOrigin);

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;
  virtual RawColorRenderImageQuantity* setEnabled(bool newEnabled) override;

  virtual std::string niceName() override;

  render::ManagedBuffer<glm::vec3> colors;

  // == Setters and getters


protected:
  // === Visualization parameters

  // Store the raw data
  std::vector<glm::vec3> colorsData;

  // === Render data
  std::shared_ptr<render::ShaderProgram> program;

  // === Helpers
  void prepare();
};


} // namespace polyscope
