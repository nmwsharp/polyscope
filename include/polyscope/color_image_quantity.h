// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/image_quantity_base.h"

#include <vector>

namespace polyscope {

class ColorImageQuantity : public ImageQuantity {

  // TODO this should probably inherit from ColorQuantity

public:
  ColorImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY, const std::vector<glm::vec4>& data,
                     ImageOrigin imageOrigin);

  virtual void buildCustomUI() override;

  virtual void refresh() override;

  virtual std::string niceName() override;

  render::ManagedBuffer<glm::vec4> colors;

  // == Setters and getters

  virtual ColorImageQuantity* setEnabled(bool newEnabled) override;


protected:
  std::vector<glm::vec4> colorsData;

  // rendering internals
  std::shared_ptr<render::ShaderProgram> fullscreenProgram, billboardProgram;
  void prepareFullscreen();
  void prepareBillboard();

  virtual void showFullscreen() override;
  virtual void showInImGuiWindow() override;
  virtual void showInBillboard(glm::vec3 center, glm::vec3 upVec, glm::vec3 rightVec) override;
};


} // namespace polyscope
