// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/polyscope.h"

#include "polyscope/render_image_quantity_base.h"

#include "imgui.h"

namespace polyscope {


RenderImageQuantityBase::RenderImageQuantityBase(Structure& parent_, std::string name, size_t dimX_, size_t dimY_,
                                                 const std::vector<float>& depthData_,
                                                 const std::vector<glm::vec3>& normalData_, ImageOrigin imageOrigin_)
    : FloatingQuantity(name, parent_), depths(this, uniquePrefix() + "depths", depthsData),
      normals(this, uniquePrefix() + "normals", normalsData), dimX(dimX_), dimY(dimY_),
      hasNormals(normalData_.size() > 0), imageOrigin(imageOrigin_), depthsData(depthData_), normalsData(normalData_),
      material(uniquePrefix() + "material", "clay"), transparency(uniquePrefix() + "transparency", 1.0),
      allowFullscreenCompositing(uniquePrefix() + "allowFullscreenCompositing", false) {
  depths.setTextureSize(dimX, dimY);
  if (hasNormals) {
    normals.setTextureSize(dimX, dimY);
  }
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

  if (ImGui::MenuItem("Allow fullscreen compositing", NULL, getAllowFullscreenCompositing())) {
    setAllowFullscreenCompositing(!getAllowFullscreenCompositing());
  }
}

void RenderImageQuantityBase::updateBaseBuffers(const std::vector<float>& newDepthData,
                                                const std::vector<glm::vec3>& newNormalData) {
  if (!newDepthData.empty()) {
    depths.ensureHostBufferAllocated();
    depths.data = newDepthData;
    depths.markHostBufferUpdated();
  }

  if (!newNormalData.empty()) {
    normals.ensureHostBufferAllocated();
    normals.data = newNormalData;
    normals.markHostBufferUpdated();
  }

  requestRedraw();
}

void RenderImageQuantityBase::setRenderImageUniforms(render::ShaderProgram& program, bool withTonemap) {
  parent.setStructureUniforms(program);

  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);

  program.setUniform("u_projMatrix", glm::value_ptr(P));
  program.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  program.setUniform("u_viewport", render::engine->getCurrentViewport());
  program.setUniform("u_textureTransparency", transparency.get());
  if (program.hasUniform("u_transparency")) {
    program.setUniform("u_transparency", 1.0f);
  }

  if (withTonemap) {
    render::engine->setTonemapUniforms(program);
  }
}

void RenderImageQuantityBase::drawPickDelayed() {
  if (!isEnabled()) return;

  if (!pickProgram) preparePick();

  // set uniforms
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);

  pickProgram->setUniform("u_projMatrix", glm::value_ptr(P));
  pickProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  pickProgram->setUniform("u_viewport", render::engine->getCurrentViewport());
  pickProgram->setUniform("u_textureTransparency", 1.0);
  pickProgram->setUniform("u_color", pickColor);

  // draw
  pickProgram->draw();
}

void RenderImageQuantityBase::preparePick() {

  // Request pick indices
  size_t pickCount = 1;
  size_t pickStart = pick::requestPickBufferRange(this, pickCount);
  pickColor = pick::indToVec(pickStart);

  // Create the sourceProgram
  // clang-format off
  pickProgram = render::engine->requestShader("TEXTURE_DRAW_RENDERIMAGE_PLAIN",
    parent.addStructureRules({
      getImageOriginRule(imageOrigin), 
      "SHADECOLOR_FROM_UNIFORM",
    }),
    render::ShaderReplacementDefaults::Pick
  );
  // clang-format on

  pickProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  pickProgram->setTextureFromBuffer("t_depth", depths.getRenderTextureBuffer().get());
}

void RenderImageQuantityBase::refresh() {
  pickProgram = nullptr;
  Quantity::refresh();
}

void RenderImageQuantityBase::disableFullscreenDrawing() {
  if (isEnabled()) {
    setEnabled(false);
  }
}

RenderImageQuantityBase* RenderImageQuantityBase::setEnabled(bool newEnabled) {
  if (newEnabled == isEnabled()) return this;
  if (newEnabled == true && !allowFullscreenCompositing.get()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  enabled = newEnabled;
  requestRedraw();
  return this;
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

RenderImageQuantityBase* RenderImageQuantityBase::setAllowFullscreenCompositing(bool newVal) {
  allowFullscreenCompositing = newVal;
  requestRedraw();
  return this;
}

bool RenderImageQuantityBase::getAllowFullscreenCompositing() { return allowFullscreenCompositing.get(); }


} // namespace polyscope
