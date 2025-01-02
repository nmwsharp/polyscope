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
                            const std::vector<float>& scalarData, ImageOrigin imageOrigin, DataType dataType);

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;

  virtual std::string niceName() override;

  // == Setters and getters

  template <typename T1, typename T2, typename T3>
  void updateBuffers(const T1& depthData, const T2& normalData, const T3& scalarData);

protected:
  // === Visualization parameters

  // === Render data
  std::shared_ptr<render::ShaderProgram> program;

  // === Helpers
  void prepare();
};

template <typename T1, typename T2, typename T3>
void ScalarRenderImageQuantity::updateBuffers(const T1& depthData, const T2& normalData, const T3& scalarData) {

  validateSize(depthData, dimX * dimY, "scalar render image depth data " + name);
  validateSize(normalData, {dimX * dimY, 0}, "scalar render image normal data " + name);
  validateSize(scalarData, dimX * dimY, "scalar render image color data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec3> standardNormal(standardizeVectorArray<glm::vec3, 3>(normalData));
  std::vector<float> standardScalar(standardizeArray<float>(scalarData));

  values.data = standardScalar;
  values.markHostBufferUpdated();

  updateBaseBuffers(standardDepth, standardNormal);
}


} // namespace polyscope
