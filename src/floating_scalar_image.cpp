// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/polyscope.h"

#include "polyscope/floating_scalar_image.h"

#include "imgui.h"

namespace polyscope {


FloatingScalarImageQuantity::FloatingScalarImageQuantity(Structure& parent_, std::string name,
                                                         size_t dimX, size_t dimY, const std::vector<double>& data,
                                                         DataType dataType)
    : FloatingQuantity(name, parent_), ImageScalarArtist(*this, name, dimX, dimY, data, dataType),
      showFullscreen(uniquePrefix() + "showFullscreen", false) {}

size_t FloatingScalarImageQuantity::nPix() { return dimX * dimY; }

void FloatingScalarImageQuantity::draw() {
  if (!isEnabled()) return;
  ImageScalarArtist::renderSource();
}

void FloatingScalarImageQuantity::drawDelayed() {
  if (!isEnabled()) return;
  if (getShowFullscreen()) {
    ImageScalarArtist::showFullscreen();
  }
}

void FloatingScalarImageQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    buildScalarOptionsUI();

    if (ImGui::MenuItem("Show fullscreen", NULL, showFullscreen.get())) setShowFullscreen(!getShowFullscreen());

    ImGui::EndPopup();
  }

  buildScalarUI();

  if (getShowFullscreen()) {
    if (ImGui::SliderFloat("transparency", &transparency.get(), 0.f, 1.f)) {
      transparency.manuallyChanged();
      requestRedraw();
    }
  }

  if (isEnabled() && parent.isEnabled()) {
    if (!getShowFullscreen()) {
      ImageScalarArtist::showInImGuiWindow();
    }
  }
}


void FloatingScalarImageQuantity::refresh() {
  ImageScalarArtist::refresh();
  Quantity::refresh();
}

std::string FloatingScalarImageQuantity::niceName() { return name + " (scalar image)"; }

FloatingScalarImageQuantity* FloatingScalarImageQuantity::setEnabled(bool newEnabled) {
  if (newEnabled == isEnabled()) return this;
  if (newEnabled == true && getShowFullscreen()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  enabled = newEnabled;
  requestRedraw();
  return this;
}

void FloatingScalarImageQuantity::disableFullscreenDrawing() {
  if (getShowFullscreen() && isEnabled() && parent.isEnabled()) {
    setEnabled(false);
  }
}


void FloatingScalarImageQuantity::setShowFullscreen(bool newVal) {
  if (newVal && isEnabled()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  showFullscreen = newVal;
  requestRedraw();
}
bool FloatingScalarImageQuantity::getShowFullscreen() { return showFullscreen.get(); }

} // namespace polyscope
