#pragma once

#include "polyscope/fullscreen_artist.h"
#include "polyscope/image_scalar_artist.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/scalar_quantity.h"

#include <vector>

namespace polyscope {

class FloatingRenderImageQuantity : public FloatingQuantity,
                                    public ImageScalarArtist<FloatingRenderImageQuantity>,
                                    public FullscreenArtist {

public:
  FloatingRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                              const std::vector<double>& depthData,
                              const std::vector<glm::vec3>& normalData,
                              );

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;
  virtual FloatingRenderImageQuantity* setEnabled(bool newEnabled) override;

  virtual std::string niceName() override;

  virtual void disableFullscreenDrawing() override;

  size_t nPix();


  // == Setters and getters

  // set the base color of the points
  FloatingRenderImageQuantity* setColor(glm::vec3 newVal);
  glm::vec3 getColor();

  // Material
  FloatingRenderImageQuantity* setMaterial(std::string name);
  std::string getMaterial();

protected:
  
  const size_t dimX, dimY;

  // === Visualization parameters
  PersistentValue<glm::vec3> color;
  PersistentValue<std::string> material;
  PersistentValue<float> transparency;


};


} // namespace polyscope
