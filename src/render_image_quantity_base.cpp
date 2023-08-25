// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/polyscope.h"

#include "polyscope/render_image_quantity_base.h"

#include "imgui.h"

namespace polyscope {


RenderImageQuantityBase::RenderImageQuantityBase(Structure& parent_, std::string name, size_t dimX_, size_t dimY_,
                                                 const std::vector<float>& depthData_,
                                                 const std::vector<glm::vec3>& normalData_, ImageOrigin imageOrigin_)
    : FloatingQuantity(name, parent_), depths(this, uniquePrefix() + "depths", depthsData),
      normals(this, uniquePrefix() + "normals", normalsData), dimX(dimX_), dimY(dimY_), imageOrigin(imageOrigin_),
      depthsData(depthData_), normalsData(normalData_), material(uniquePrefix() + "#material", "clay"),
      transparency(uniquePrefix() + "#transparency", 1.0) {
  depths.setTextureSize(dimX, dimY);
  normals.setTextureSize(dimX, dimY);
}

size_t RenderImageQuantityBase::nPix() { return dimX * dimY; }

void RenderImageQuantityBase::addOptionsPopupEntries() {

  if (ImGui::BeginMenu("Transparency")) {
    if (ImGui::SliderFloat("Alpha", &transparency.get(), 0., 1., "%.3f")) setTransparency(transparency.get());
    ImGui::TextUnformatted("Note: Change the transparency mode");
    ImGui::TextUnformatted("      in Appearance --> Transparency.");
    ImGui::TextUnformatted("Current mode: ");
    ImGui::SameLine();
    ImGui::TextUnformatted(modeName(render::engine->getTransparencyMode()).c_str());
    ImGui::EndMenu();
  }

  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get()); // trigger the other updates that happen on set()
  }
}

void RenderImageQuantityBase::updateBaseBuffers(const std::vector<float>& newDepthData,
                                                const std::vector<glm::vec3>& newNormalData) {
  if (!newDepthData.empty()) {
    depths.data = newDepthData;
    depths.markHostBufferUpdated();
  }

  if (!newNormalData.empty()) {
    normals.data = newNormalData;
    normals.markHostBufferUpdated();
  }

  requestRedraw();
}

void RenderImageQuantityBase::refresh() { Quantity::refresh(); }


RenderImageQuantityBase* RenderImageQuantityBase::setMaterial(std::string m) {
  material = m;
  refresh();
  requestRedraw();
  return this;
}
std::string RenderImageQuantityBase::getMaterial() { return material.get(); }


RenderImageQuantityBase* RenderImageQuantityBase::setTransparency(float newVal) {
  transparency = newVal;

  // DON'T do this for images, unlike other structures, because they just get drawn on top anyway
  // if (newVal < 1. && options::transparencyMode == TransparencyMode::None) {
  // options::transparencyMode = TransparencyMode::Pretty;
  //}
  requestRedraw();

  return this;
}
float RenderImageQuantityBase::getTransparency() { return transparency.get(); }

} // namespace polyscope
