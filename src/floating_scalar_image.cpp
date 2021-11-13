// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/floating_scalar_image.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {


FloatingScalarImageQuantity::FloatingScalarImageQuantity(FloatingQuantityStructure& parent_, std::string name,
                                                         size_t dimX, size_t dimY, const std::vector<double>& data,
                                                         DataType dataType)
    : FloatingQuantity(name, parent_, false), ImageScalarArtist(*this, name, dimX, dimY, data, dataType),
      showFullscreen(uniquePrefix() + "showFullscreen", false) {}

size_t FloatingScalarImageQuantity::nPix() { return dimX * dimY; }

void FloatingScalarImageQuantity::draw() {
  if (!isEnabled()) return;
  ImageScalarArtist::renderSource();
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


void FloatingScalarImageQuantity::setShowFullscreen(bool newVal) {
  showFullscreen = newVal;
  requestRedraw();
}
bool FloatingScalarImageQuantity::getShowFullscreen() { return showFullscreen.get(); }

} // namespace polyscope
