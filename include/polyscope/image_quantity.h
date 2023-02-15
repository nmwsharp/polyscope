#pragma once

#include "polyscope/polyscope.h"

#include "polyscope/floating_quantity.h"
#include "polyscope/fullscreen_artist.h"

#include <vector>

namespace polyscope {

class ImageQuantity : public FloatingQuantity, public FullscreenArtist {

public:
  ImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY, ImageOrigin imageOrigin);


  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void disableFullscreenDrawing() override;

  // render to a rectangle in 3D
  // note that the magnitudes of upVec matters, it determines the size of the billboard in world space
  // the magnitude of rightVec is ignored and scaled to match the aspect ratio of the image
  virtual void showInBillboard(glm::vec3 center, glm::vec3 upVec, glm::vec3 rightVec) = 0;

  Structure& parent;

  // == Setters and getters

  size_t nPix();

  void setShowFullscreen(bool newVal);
  bool getShowFullscreen();
  
  void setShowInImGuiWindow(bool newVal);
  bool getShowInImGuiWindow();

  void setTransparency(float newVal);
  float getTransparency();

protected:
  // === Visualization parameters
  const size_t dimX, dimY;
  ImageOrigin imageOrigin;
  PersistentValue<float> transparency;
  PersistentValue<bool> isShowingFullscreen, isShowingImGuiWindow;

  // render the image fullscreen
  virtual void showFullscreen() = 0;

  // build a floating imgui window showing the texture
  virtual void showInImGuiWindow() = 0;

  // you MUST call this at draw time if you intend to call showInImGuiWindow() later
  virtual void renderIntermediate();
};

} // namespace polyscope
