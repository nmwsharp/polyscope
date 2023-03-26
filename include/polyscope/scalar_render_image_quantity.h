// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/render_image_quantity_base.h"
#include "polyscope/scalar_quantity.h"

#include <vector>

namespace polyscope {

class ScalarRenderImageQuantity : public RenderImageQuantityBase, public ScalarQuantity<ScalarRenderImageQuantity> {

public:
  ScalarRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                            const std::vector<float>& depthData, const std::vector<glm::vec3>& normalData,
                            const std::vector<double>& scalarData, ImageOrigin imageOrigin, DataType dataType);

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;
  virtual ScalarRenderImageQuantity* setEnabled(bool newEnabled) override;

  virtual std::string niceName() override;

  // == Setters and getters


protected:
  // === Visualization parameters

  // === Render data
  std::shared_ptr<render::TextureBuffer> textureScalar;
  std::shared_ptr<render::ShaderProgram> program;

  // === Helpers
  void prepare();
};


} // namespace polyscope
