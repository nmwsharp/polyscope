// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render_image_quantity_base.h"
#include "polyscope/types.h"

#include <vector>

namespace polyscope {

class RawColorAlphaRenderImageQuantity : public RenderImageQuantityBase {

public:
  RawColorAlphaRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                   const std::vector<float>& depthData, const std::vector<glm::vec4>& colorsData,
                                   ImageOrigin imageOrigin);

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;

  virtual std::string niceName() override;

  render::ManagedBuffer<glm::vec4> colors;

  template <typename T1, typename T2>
  void updateBuffers(const T1& depthData, const T2& colorsData);

  // == Setters and getters

  RawColorAlphaRenderImageQuantity* setIsPremultiplied(bool val);
  bool getIsPremultiplied();

protected:
  // Store the raw data
  std::vector<glm::vec4> colorsData;

  // === Visualization parameters
  PersistentValue<bool> isPremultiplied;

  // === Render data
  std::shared_ptr<render::ShaderProgram> program;

  // === Helpers
  void prepare();
};

template <typename T1, typename T2>
void RawColorAlphaRenderImageQuantity::updateBuffers(const T1& depthData, const T2& colorsData) {

  validateSize(depthData, dimX * dimY, "color render image depth data " + name);
  validateSize(colorsData, dimX * dimY, "color render image color data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec3> standardNormal;
  std::vector<glm::vec4> standardColor(standardizeVectorArray<glm::vec4, 4>(colorsData));

  colors.data = standardColor;
  colors.markHostBufferUpdated();

  updateBaseBuffers(standardDepth, standardNormal);
}

} // namespace polyscope
