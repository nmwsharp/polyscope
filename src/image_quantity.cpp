// Copyright 2018-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/image_quantity.h"

#include "imgui.h"

namespace polyscope {


ImageQuantity::ImageQuantity(Structure& parent_, std::string name_, size_t dimX_, size_t dimY_,
                             ImageOrigin imageOrigin_)
    : FloatingQuantity(name_, parent_), parent(parent_), dimX(dimX_), dimY(dimY_), imageOrigin(imageOrigin_),
      transparency(uniquePrefix() + "transparency", 1.0),
      isShowingFullscreen(uniquePrefix() + "isShowingFullscreen", false),
      isShowingImGuiWindow(uniquePrefix() + "isShowingImGuiWindow", true) {}

void ImageQuantity::draw() {
  if (!isEnabled()) return;

  if (getShowInImGuiWindow()) {
    renderIntermediate();
  }
}

void ImageQuantity::drawDelayed() {
  if (!isEnabled()) return;
  if (getShowFullscreen()) {
    showFullscreen();
  }
}

void ImageQuantity::renderIntermediate() {
  // nothing by default, subclasses override
}

void ImageQuantity::disableFullscreenDrawing() {
  if (getShowFullscreen() && isEnabled() && parent.isEnabled()) {
    setEnabled(false);
  }
}

size_t ImageQuantity::nPix() { return dimX * dimY; }

void ImageQuantity::setShowFullscreen(bool newVal) {
  if (newVal && isEnabled()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  isShowingFullscreen = newVal;
  requestRedraw();
}
bool ImageQuantity::getShowFullscreen() { return isShowingFullscreen.get(); }

void ImageQuantity::setShowInImGuiWindow(bool newVal) {
  isShowingImGuiWindow = newVal;
  requestRedraw();
}
bool ImageQuantity::getShowInImGuiWindow() { return isShowingImGuiWindow.get(); }

void ImageQuantity::setTransparency(float newVal) {
  transparency = newVal;
  requestRedraw();
}

float ImageQuantity::getTransparency() { return transparency.get(); }


} // namespace polyscope
