#pragma once

#include "polyscope/fullscreen_artist.h"
#include "polyscope/image_color_artist.h"

#include <vector>

namespace polyscope {

class FloatingColorImageQuantity : public FloatingQuantity, public ImageColorArtist, public FullscreenArtist {

public:
  FloatingColorImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                             const std::vector<glm::vec4>& data);

  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void buildCustomUI() override;

  virtual void refresh() override;
  virtual FloatingColorImageQuantity* setEnabled(bool newEnabled) override;

  virtual std::string niceName() override;

  virtual void disableFullscreenDrawing() override;


  size_t nPix();

  Structure& parent;

  // == Setters and getters

  void setShowFullscreen(bool newVal);
  bool getShowFullscreen();

protected:
  // === Visualization parameters
  PersistentValue<bool> showFullscreen;
};


} // namespace polyscope
