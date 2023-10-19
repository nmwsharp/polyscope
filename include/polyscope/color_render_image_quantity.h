// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render_image_quantity_base.h"
#include "polyscope/types.h"

#include <vector>

namespace polyscope {

class ColorRenderImageQuantity : public RenderImageQuantityBase {

  // TODO this should probably inherit from ColorQuantity

public:
  ColorRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                           const std::vector<float>& depthData, const std::vector<glm::vec3>& normalData,
                           const std::vector<glm::vec3>& colorsData, ImageOrigin imageOrigin);

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;

  virtual std::string niceName() override;

  render::ManagedBuffer<glm::vec3> colors;

  template <typename T1, typename T2, typename T3>
  void updateBuffers(const T1& depthData, const T2& normalData, const T3& colorsData);

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


template <typename T1, typename T2, typename T3>
void ColorRenderImageQuantity::updateBuffers(const T1& depthData, const T2& normalData, const T3& colorsData) {

  validateSize(depthData, dimX * dimY, "color render image depth data " + name);
  validateSize(normalData, {dimX * dimY, 0}, "color render image normal data " + name);
  validateSize(colorsData, dimX * dimY, "color render image color data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec3> standardNormal(standardizeVectorArray<glm::vec3, 3>(normalData));
  std::vector<glm::vec3> standardColor(standardizeVectorArray<glm::vec3, 3>(colorsData));

  colors.data = standardColor;
  colors.markHostBufferUpdated();

  updateBaseBuffers(standardDepth, standardNormal);
}


} // namespace polyscope
