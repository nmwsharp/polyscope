// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/polyscope.h"

#include "polyscope/floating_color_image.h"

#include "imgui.h"

namespace polyscope {


FloatingColorImageQuantity::FloatingColorImageQuantity(Structure& parent_, std::string name,
                                                       size_t dimX, size_t dimY, const std::vector<glm::vec4>& data)
    : FloatingQuantity(name, parent_), ImageColorArtist(name, uniquePrefix(), dimX, dimY, data), parent(parent_),
      showFullscreen(uniquePrefix() + "showFullscreen", false) {}

size_t FloatingColorImageQuantity::nPix() { return dimX * dimY; }

void FloatingColorImageQuantity::draw() {
  if (!isEnabled()) return;
  ImageColorArtist::renderSource();
}

void FloatingColorImageQuantity::drawDelayed() {
  if (!isEnabled()) return;
  if (getShowFullscreen()) {
    ImageColorArtist::showFullscreen();
  }
}


void FloatingColorImageQuantity::buildCustomUI() {
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
    if (ImGui::SliderFloat("transparency", &transparency.get(), 0.f, 1.f)) {
      transparency.manuallyChanged();
      requestRedraw();
    }
  }

  if (isEnabled() && parent.isEnabled()) {
    if (!getShowFullscreen()) {
      ImageColorArtist::showInImGuiWindow();
    }
  }
}


void FloatingColorImageQuantity::refresh() {
  ImageColorArtist::refresh();
  Quantity::refresh();
}

std::string FloatingColorImageQuantity::niceName() { return name + " (color image)"; }

FloatingColorImageQuantity* FloatingColorImageQuantity::setEnabled(bool newEnabled) {
  if (newEnabled == isEnabled()) return this;
  if (newEnabled == true && getShowFullscreen()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  enabled = newEnabled;
  requestRedraw();
  return this;
}

void FloatingColorImageQuantity::disableFullscreenDrawing() {
  if (getShowFullscreen() && isEnabled() && parent.isEnabled()) {
    setEnabled(false);
  }
}


void FloatingColorImageQuantity::setShowFullscreen(bool newVal) {
  if (newVal && isEnabled()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  showFullscreen = newVal;
  requestRedraw();
}
bool FloatingColorImageQuantity::getShowFullscreen() { return showFullscreen.get(); }

} // namespace polyscope
