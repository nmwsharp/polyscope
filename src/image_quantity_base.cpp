// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/image_quantity_base.h"

#include "polyscope/camera_view.h"

#include "imgui.h"

namespace polyscope {


ImageQuantity::ImageQuantity(Structure& parent_, std::string name_, size_t dimX_, size_t dimY_,
                             ImageOrigin imageOrigin_)
    : FloatingQuantity(name_, parent_), parent(parent_), dimX(dimX_), dimY(dimY_), imageOrigin(imageOrigin_),
      transparency(uniquePrefix() + "transparency", 1.0),
      isShowingFullscreen(uniquePrefix() + "isShowingFullscreen", false),
      isShowingImGuiWindow(uniquePrefix() + "isShowingImGuiWindow", true),
      isShowingCameraBillboard(uniquePrefix() + "isCameraBillboard", false) {

  parentStructureCameraView = dynamic_cast<CameraView*>(&parent);
  if (parentIsCameraView()) {
    // different defaults for camera views
    isShowingCameraBillboard.setPassive(true);
    isShowingImGuiWindow.setPassive(false);
  }
}

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

  if (getShowInCameraBillboard()) {
    glm::vec3 billboardCenter, billboardUp, billboardRight;
    std::tie(billboardCenter, billboardUp, billboardRight) = parentStructureCameraView->getFrameBillboardGeometry();

    showInBillboard(billboardCenter, billboardUp, billboardRight);
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

void ImageQuantity::setShowInCameraBillboard(bool newVal) {
  if (!parentIsCameraView()) newVal = false; // don't allow setting to true if parent is not camera
  isShowingCameraBillboard = newVal;
  requestRedraw();
}
bool ImageQuantity::getShowInCameraBillboard() { return isShowingCameraBillboard.get(); }

void ImageQuantity::setTransparency(float newVal) {
  transparency = newVal;
  requestRedraw();
}

float ImageQuantity::getTransparency() { return transparency.get(); }

bool ImageQuantity::parentIsCameraView() { return parentStructureCameraView != nullptr; }

void ImageQuantity::buildImageOptionsUI() {

  if (ImGui::MenuItem("Show in ImGui window", NULL, getShowInImGuiWindow()))
    setShowInImGuiWindow(!getShowInImGuiWindow());
  if (ImGui::MenuItem("Show fullscreen", NULL, getShowFullscreen())) setShowFullscreen(!getShowFullscreen());

  if (parentIsCameraView()) {
    if (ImGui::MenuItem("Show in camera billboard", NULL, getShowInCameraBillboard()))
      setShowInCameraBillboard(!getShowInCameraBillboard());
  }

  if (ImGui::SliderFloat("transparency", &transparency.get(), 0, 1., "%.3f")) {
    transparency.manuallyChanged();
    requestRedraw();
  }
}

void ImageQuantity::buildImageUI() {

  if (getShowFullscreen()) {

    ImGui::PushItemWidth(100);
    if (ImGui::SliderFloat("transparency", &transparency.get(), 0.f, 1.f)) {
      transparency.manuallyChanged();
      requestRedraw();
    }
    ImGui::PopItemWidth();
  }

  if (isEnabled() && parent.isEnabled()) {
    if (getShowInImGuiWindow()) {
      showInImGuiWindow();
    }
  }
}


} // namespace polyscope
