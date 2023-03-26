// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/polyscope.h"

#include "polyscope/depth_render_image_quantity.h"

#include "imgui.h"

namespace polyscope {


DepthRenderImageQuantity::DepthRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                                   const std::vector<float>& depthData,
                                                   const std::vector<glm::vec3>& normalData, ImageOrigin imageOrigin_)
    : RenderImageQuantityBase(parent_, name, dimX, dimY, depthData, normalData, imageOrigin_),
      color(uniquePrefix() + "#color", getNextUniqueColor()) {}

void DepthRenderImageQuantity::draw() {}

void DepthRenderImageQuantity::drawDelayed() {
  if (!isEnabled()) return;

  if (!program) prepare();

  // set uniforms
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);

  program->setUniform("u_projMatrix", glm::value_ptr(P));
  program->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  program->setUniform("u_viewport", render::engine->getCurrentViewport());
  program->setUniform("u_baseColor", color.get());
  program->setUniform("u_transparency", transparency.get());

  // make sure we have actual depth testing enabled
  render::engine->setDepthMode(DepthMode::LEqual);
  // render::engine->applyTransparencySettings();
  render::engine->setBlendMode(BlendMode::Over);

  // draw
  program->draw();
}

void DepthRenderImageQuantity::buildCustomUI() {
  ImGui::SameLine();

  if (ImGui::ColorEdit3("color", &color.get()[0], ImGuiColorEditFlags_NoInputs)) {
    setColor(getColor());
  }
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    RenderImageQuantityBase::addOptionsPopupEntries();

    ImGui::EndPopup();
  }
}


void DepthRenderImageQuantity::refresh() {
  program = nullptr;
  RenderImageQuantityBase::refresh();
}


void DepthRenderImageQuantity::prepare() {
  prepareGeometryBuffers();

  // no extra data to push for this one

  // Create the sourceProgram
  program = render::engine->requestShader("TEXTURE_DRAW_RENDERIMAGE_PLAIN",
                                          {getImageOriginRule(imageOrigin), "LIGHT_MATCAP", "SHADE_BASECOLOR"},
                                          render::ShaderReplacementDefaults::Process);

  program->setAttribute("a_position", render::engine->screenTrianglesCoords());
  program->setTextureFromBuffer("t_depth", textureDepth.get());
  program->setTextureFromBuffer("t_normal", textureNormal.get());
  render::engine->setMaterial(*program, material.get());
}


std::string DepthRenderImageQuantity::niceName() { return name + " (render image)"; }

DepthRenderImageQuantity* DepthRenderImageQuantity::setEnabled(bool newEnabled) {
  enabled = newEnabled;
  requestRedraw();
  return this;
}

DepthRenderImageQuantity* DepthRenderImageQuantity::setColor(glm::vec3 newVal) {
  color = newVal;
  polyscope::requestRedraw();
  return this;
}
glm::vec3 DepthRenderImageQuantity::getColor() { return color.get(); }


// Instantiate a construction helper which is used to avoid header dependencies. See forward declaration and note in
// structure.ipp.
DepthRenderImageQuantity* createDepthRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                 const std::vector<float>& depthData,
                                                 const std::vector<glm::vec3>& normalData, ImageOrigin imageOrigin) {
  return new DepthRenderImageQuantity(parent, name, dimX, dimY, depthData, normalData, imageOrigin);
}

} // namespace polyscope
