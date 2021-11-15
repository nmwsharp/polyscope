#pragma once

#include "polyscope/floating_quantity_structure.h"
#include "polyscope/fullscreen_artist.h"
#include "polyscope/image_scalar_artist.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/scalar_quantity.h"

#include <vector>

namespace polyscope {

class FloatingScalarImageQuantity : public FloatingQuantity,
                                    public ImageScalarArtist<FloatingScalarImageQuantity>,
                                    public FullscreenArtist {

public:
  FloatingScalarImageQuantity(FloatingQuantityStructure& parent_, std::string name, size_t dimX, size_t dimY,
                              const std::vector<double>& data, DataType dataType);

  virtual void draw() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;
  virtual FloatingScalarImageQuantity* setEnabled(bool newEnabled) override;

  virtual std::string niceName() override;

  virtual void disableFullscreenDrawing() override;

  size_t nPix();

  // == Setters and getters

  void setShowFullscreen(bool newVal);
  bool getShowFullscreen();

protected:
  // === Visualization parameters
  PersistentValue<bool> showFullscreen;

  void createPointProgram();
  std::shared_ptr<render::ShaderProgram> pointProgram;
};


} // namespace polyscope
