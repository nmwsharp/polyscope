// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/polyscope.h"

#include "polyscope/scalar_image_quantity.h"

#include "imgui.h"

namespace polyscope {


ScalarImageQuantity::ScalarImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                         const std::vector<double>& data, DataType dataType)
    : FloatingQuantity(name, parent_), ImageScalarArtist(*this, name, dimX, dimY, data, dataType),
      showFullscreen(uniquePrefix() + "showFullscreen", false) {}

size_t ScalarImageQuantity::nPix() { return dimX * dimY; }

void ScalarImageQuantity::draw() {
  if (!isEnabled()) return;
  ImageScalarArtist::renderSource();
}

void ScalarImageQuantity::drawDelayed() {
  if (!isEnabled()) return;
  if (getShowFullscreen()) {
    ImageScalarArtist::showFullscreen();
  }
}

void ScalarImageQuantity::buildCustomUI() {
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

    ImGui::PushItemWidth(100);
    if (ImGui::SliderFloat("transparency", &transparency.get(), 0.f, 1.f)) {
      transparency.manuallyChanged();
      requestRedraw();
    }
    ImGui::PopItemWidth();
  }

  if (isEnabled() && parent.isEnabled()) {
    if (!getShowFullscreen()) {
      ImageScalarArtist::showInImGuiWindow();
    }
  }
}


void ScalarImageQuantity::refresh() {
  ImageScalarArtist::refresh();
  Quantity::refresh();
}

std::string ScalarImageQuantity::niceName() { return name + " (scalar image)"; }

ScalarImageQuantity* ScalarImageQuantity::setEnabled(bool newEnabled) {
  if (newEnabled == isEnabled()) return this;
  if (newEnabled == true && getShowFullscreen()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  enabled = newEnabled;
  requestRedraw();
  return this;
}

void ScalarImageQuantity::disableFullscreenDrawing() {
  if (getShowFullscreen() && isEnabled() && parent.isEnabled()) {
    setEnabled(false);
  }
}


void ScalarImageQuantity::setShowFullscreen(bool newVal) {
  if (newVal && isEnabled()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  showFullscreen = newVal;
  requestRedraw();
}
bool ScalarImageQuantity::getShowFullscreen() { return showFullscreen.get(); }

// Instantiate a construction helper which is used to avoid header dependencies. See forward declaration and note in
// structure.ipp.
ScalarImageQuantity* createScalarImageQuantity(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                               const std::vector<double>& data, DataType dataType) {
  return new ScalarImageQuantity(parent, name, dimX, dimY, data, dataType);
}

} // namespace polyscope
