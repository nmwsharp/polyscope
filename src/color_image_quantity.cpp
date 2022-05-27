// Copyright 2018-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/polyscope.h"

#include "polyscope/color_image_quantity.h"

#include "imgui.h"

namespace polyscope {


ColorImageQuantity::ColorImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                       const std::vector<glm::vec4>& data)
    : FloatingQuantity(name, parent_), ImageColorArtist(name, uniquePrefix(), dimX, dimY, data), parent(parent_),
      showFullscreen(uniquePrefix() + "showFullscreen", false) {}

size_t ColorImageQuantity::nPix() { return dimX * dimY; }

void ColorImageQuantity::draw() {
  if (!isEnabled()) return;
  ImageColorArtist::renderSource();
}

void ColorImageQuantity::drawDelayed() {
  if (!isEnabled()) return;
  if (getShowFullscreen()) {
    ImageColorArtist::showFullscreen();
  }
}


void ColorImageQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    if (ImGui::MenuItem("Show fullscreen", NULL, showFullscreen.get())) setShowFullscreen(!getShowFullscreen());

    ImGui::EndPopup();
  }

  if (getShowFullscreen()) {
    ImGui::PushItemWidth(100);
    if (ImGui::SliderFloat("transparency", &transparency.get(), 0.f, 1.f)) {
      transparency.manuallyChanged();
      requestRedraw();
    }
    ImGui::PopItemWidth();
  }

  if (isEnabled() && parent.isEnabled()) {
    if (!getShowFullscreen()) {
      ImageColorArtist::showInImGuiWindow();
    }
  }
}


void ColorImageQuantity::refresh() {
  ImageColorArtist::refresh();
  Quantity::refresh();
}

std::string ColorImageQuantity::niceName() { return name + " (color image)"; }

ColorImageQuantity* ColorImageQuantity::setEnabled(bool newEnabled) {
  if (newEnabled == isEnabled()) return this;
  if (newEnabled == true && getShowFullscreen()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  enabled = newEnabled;
  requestRedraw();
  return this;
}

void ColorImageQuantity::disableFullscreenDrawing() {
  if (getShowFullscreen() && isEnabled() && parent.isEnabled()) {
    setEnabled(false);
  }
}


void ColorImageQuantity::setShowFullscreen(bool newVal) {
  if (newVal && isEnabled()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  showFullscreen = newVal;
  requestRedraw();
}
bool ColorImageQuantity::getShowFullscreen() { return showFullscreen.get(); }


// Instantiate a construction helper which is used to avoid header dependencies. See forward declaration and note in
// structure.ipp.
ColorImageQuantity* createColorImageQuantity(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                     const std::vector<glm::vec4>& data) {
  return new ColorImageQuantity(parent, name, dimX, dimY, data);
}


} // namespace polyscope
