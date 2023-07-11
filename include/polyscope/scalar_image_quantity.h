// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/fullscreen_artist.h"
#include "polyscope/image_quantity_base.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/scalar_quantity.h"

#include <vector>

namespace polyscope {

class ScalarImageQuantity : public ImageQuantity, public ScalarQuantity<ScalarImageQuantity> {

public:
  ScalarImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY, const std::vector<double>& data,
                      ImageOrigin imageOrigin, DataType dataType);

  virtual void buildCustomUI() override;

  virtual void refresh() override;

  virtual std::string niceName() override;

  // == Setters and getters

  virtual ScalarImageQuantity* setEnabled(bool newEnabled) override;

protected:
  // rendering internals
  std::shared_ptr<render::TextureBuffer> textureRaw, textureIntermediateRendered;
  std::shared_ptr<render::ShaderProgram> fullscreenProgram, billboardProgram;
  std::shared_ptr<render::FrameBuffer> framebufferIntermediate;

  void prepareFullscreen();
  void prepareIntermediateRender();
  void prepareBillboard();
  void ensureRawTexturePopulated();

  virtual void showFullscreen() override;
  virtual void showInImGuiWindow() override;
  virtual void showInBillboard(glm::vec3 center, glm::vec3 upVec, glm::vec3 rightVec) override;
  virtual void renderIntermediate() override;
};


} // namespace polyscope
