// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render_image_quantity_base.h"

#include <vector>

namespace polyscope {

class DepthRenderImageQuantity : public RenderImageQuantityBase {

public:
  DepthRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                           const std::vector<float>& depthData, const std::vector<glm::vec3>& normalData,
                           ImageOrigin imageOrigin);

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;

  virtual std::string niceName() override;

  template <typename T1, typename T2>
  void updateBuffers(const T1& depthData, const T2& normalData);

  // == Setters and getters

  // set the base color of the rendered geometry
  DepthRenderImageQuantity* setColor(glm::vec3 newVal);
  glm::vec3 getColor();


protected:
  // === Visualization parameters
  PersistentValue<glm::vec3> color;


  // === Render data
  std::shared_ptr<render::ShaderProgram> program;

  // === Helpers
  void prepare();
};


template <typename T1, typename T2>
void DepthRenderImageQuantity::updateBuffers(const T1& depthData, const T2& normalData) {

  validateSize(depthData, dimX * dimY, "depth render image depth data " + name);
  validateSize(normalData, {dimX * dimY, 0}, "depth render image normal data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec3> standardNormal(standardizeVectorArray<glm::vec3, 3>(normalData));

  updateBaseBuffers(standardDepth, standardNormal);
}


} // namespace polyscope
