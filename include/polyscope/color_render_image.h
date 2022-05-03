#pragma once

#include "polyscope/render_image_quantity_base.h"

#include <vector>

namespace polyscope {

class ColorRenderImage : public RenderImageQuantityBase {

public:
  ColorRenderImage(Structure& parent_, std::string name, size_t dimX, size_t dimY, const std::vector<float>& depthData,
                   const std::vector<glm::vec3>& normalData, const std::vector<glm::vec3>& colorData);

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;
  virtual ColorRenderImage* setEnabled(bool newEnabled) override;

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
