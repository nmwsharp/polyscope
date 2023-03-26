// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/polyscope.h"

#include "polyscope/render_image_quantity_base.h"

#include "imgui.h"

namespace polyscope {


RenderImageQuantityBase::RenderImageQuantityBase(Structure& parent_, std::string name, size_t dimX_, size_t dimY_,
                                                 const std::vector<float>& depthData_,
                                                 const std::vector<glm::vec3>& normalData_, ImageOrigin imageOrigin_)
    : FloatingQuantity(name, parent_), dimX(dimX_), dimY(dimY_), imageOrigin(imageOrigin_), depthData(depthData_),
      normalData(normalData_), material(uniquePrefix() + "#material", "clay"),
      transparency(uniquePrefix() + "#transparency", 1.0) {}

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

void RenderImageQuantityBase::updateGeometryBuffers(const std::vector<float>& newDepthData,
                                                    const std::vector<glm::vec3>& newNormalData) {
  depthData = newDepthData;
  normalData = newNormalData;

  // if prepared, re-prepare
  // TODO do a fast update in the same buffer here
  if (textureDepth) {
    prepareGeometryBuffers();
  }
}

void RenderImageQuantityBase::prepareGeometryBuffers() {

  // == depth texture
  textureDepth = render::engine->generateTextureBuffer(TextureFormat::R32F, dimX, dimY, &depthData.front());

  // == normal texture

  // sanity check for glm struct layout
  static_assert(sizeof(glm::vec3) == sizeof(float) * 3, "glm vec padding breaks direct copy");

  textureNormal = render::engine->generateTextureBuffer(TextureFormat::RGB32F, dimX, dimY,
                                                        static_cast<float*>(&normalData.front()[0]));
}

void RenderImageQuantityBase::refresh() {
  textureDepth = nullptr;
  textureNormal = nullptr;
  Quantity::refresh();
}


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
