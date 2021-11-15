#pragma once

#include "polyscope/floating_quantity.h"
#include "polyscope/floating_quantity_structure.h"
#include "polyscope/fullscreen_artist.h"
#include "polyscope/image_color_artist.h"

#include <vector>

namespace polyscope {

class FloatingColorImageQuantity : public FloatingQuantity, public ImageColorArtist, public FullscreenArtist {

public:
  FloatingColorImageQuantity(FloatingQuantityStructure& parent_, std::string name, size_t dimX, size_t dimY,
                             const std::vector<glm::vec4>& data);

  virtual void draw() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;
  virtual FloatingColorImageQuantity* setEnabled(bool newEnabled) override;

  virtual std::string niceName() override;

  virtual void disableFullscreenDrawing() override;


  size_t nPix();

  FloatingQuantityStructure& parent;

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
