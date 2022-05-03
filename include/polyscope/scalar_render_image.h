#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/render_image_quantity_base.h"
#include "polyscope/scalar_quantity.h"

#include <vector>

namespace polyscope {

class ScalarRenderImage : public RenderImageQuantityBase, public ScalarQuantity<ScalarRenderImage> {

public:
  ScalarRenderImage(Structure& parent_, std::string name, size_t dimX, size_t dimY, const std::vector<float>& depthData,
                    const std::vector<glm::vec3>& normalData, const std::vector<double>& scalarData, DataType dataType);

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;
  virtual ScalarRenderImage* setEnabled(bool newEnabled) override;

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
