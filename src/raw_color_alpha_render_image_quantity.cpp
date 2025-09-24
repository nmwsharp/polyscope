// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/polyscope.h"

#include "polyscope/raw_color_alpha_render_image_quantity.h"

#include "imgui.h"
#include "polyscope/render/engine.h"

namespace polyscope {


RawColorAlphaRenderImageQuantity::RawColorAlphaRenderImageQuantity(Structure& parent_, std::string name, size_t dimX,
                                                                   size_t dimY, const std::vector<float>& depthData,
                                                                   const std::vector<glm::vec4>& colorsData_,
                                                                   ImageOrigin imageOrigin)
    : RenderImageQuantityBase(parent_, name, dimX, dimY, depthData, std::vector<glm::vec3>(), imageOrigin),
      colors(this, uniquePrefix() + "colors", colorsData), colorsData(colorsData_),
      isPremultiplied(uniquePrefix() + "isPremultiplied", false) {
  colors.setTextureSize(dimX, dimY);
}

void RawColorAlphaRenderImageQuantity::draw() {}

void RawColorAlphaRenderImageQuantity::drawDelayed() {
  if (!isEnabled()) return;

  if (!program) prepare();

  setRenderImageUniforms(*program, true);

  // draw
  program->draw();
}

void RawColorAlphaRenderImageQuantity::buildCustomUI() {
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


void RawColorAlphaRenderImageQuantity::refresh() {
  program = nullptr;
  RenderImageQuantityBase::refresh();
}


void RawColorAlphaRenderImageQuantity::prepare() {

  // NOTE: we use INVERSE_TONEMAP to avoid tonemapping the content, but in the presence of transparency this setup
  // cannot exactly preserve the result, since the inversion is applied before compositing but finaltonemapping is
  // applied after compositing.

  // Create the sourceProgram
  program = render::engine->requestShader(
      "TEXTURE_DRAW_RAW_RENDERIMAGE_PLAIN",
      parent.addStructureRules({getImageOriginRule(imageOrigin), "TEXTURE_SHADE_COLORALPHA", "INVERSE_TONEMAP",
                                getIsPremultiplied() ? "" : "TEXTURE_PREMULTIPLY_OUT"}),
      render::ShaderReplacementDefaults::SceneObjectNoSlice);

  program->setAttribute("a_position", render::engine->screenTrianglesCoords());
  program->setTextureFromBuffer("t_depth", depths.getRenderTextureBuffer().get());
  program->setTextureFromBuffer("t_color", colors.getRenderTextureBuffer().get());
}


std::string RawColorAlphaRenderImageQuantity::niceName() { return name + " (raw color render image)"; }

RawColorAlphaRenderImageQuantity* RawColorAlphaRenderImageQuantity::setIsPremultiplied(bool val) {
  isPremultiplied = val;
  refresh();
  return this;
}

bool RawColorAlphaRenderImageQuantity::getIsPremultiplied() { return isPremultiplied.get(); }


// Instantiate a construction helper which is used to avoid header dependencies. See forward declaration and note in
// structure.ipp.
RawColorAlphaRenderImageQuantity* createRawColorAlphaRenderImage(Structure& parent, std::string name, size_t dimX,
                                                                 size_t dimY, const std::vector<float>& depthData,
                                                                 const std::vector<glm::vec4>& colorData,
                                                                 ImageOrigin imageOrigin) {
  return new RawColorAlphaRenderImageQuantity(parent, name, dimX, dimY, depthData, colorData, imageOrigin);
}

} // namespace polyscope
