// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/polyscope.h"

#include "polyscope/raw_color_render_image_quantity.h"

#include "imgui.h"
#include "polyscope/render/engine.h"

namespace polyscope {


RawColorRenderImageQuantity::RawColorRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                                         const std::vector<float>& depthData,
                                                         const std::vector<glm::vec3>& colorsData_,
                                                         ImageOrigin imageOrigin)
    : RenderImageQuantityBase(parent_, name, dimX, dimY, depthData, std::vector<glm::vec3>(), imageOrigin),
      colors(this, uniquePrefix() + "colors", colorsData), colorsData(colorsData_) {
  colors.setTextureSize(dimX, dimY);
}

void RawColorRenderImageQuantity::draw() {}

void RawColorRenderImageQuantity::drawDelayed() {
  if (!isEnabled()) return;

  if (!program) prepare();

  setRenderImageUniforms(*program, true);

  // draw
  program->draw();
}

void RawColorRenderImageQuantity::buildCustomUI() {
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


void RawColorRenderImageQuantity::refresh() {
  program = nullptr;
  RenderImageQuantityBase::refresh();
}


void RawColorRenderImageQuantity::prepare() {

  // Create the sourceProgram
  program =
      render::engine->requestShader("TEXTURE_DRAW_RAW_RENDERIMAGE_PLAIN",
                                    parent.addStructureRules({getImageOriginRule(imageOrigin), "TEXTURE_SHADE_COLOR",
                                                              "INVERSE_TONEMAP", "PREMULTIPLY_LIT_COLOR"}),
                                    render::ShaderReplacementDefaults::SceneObjectNoSlice);

  program->setAttribute("a_position", render::engine->screenTrianglesCoords());
  program->setTextureFromBuffer("t_depth", depths.getRenderTextureBuffer().get());
  program->setTextureFromBuffer("t_color", colors.getRenderTextureBuffer().get());
}


std::string RawColorRenderImageQuantity::niceName() { return name + " (raw color render image)"; }

// Instantiate a construction helper which is used to avoid header dependencies. See forward declaration and note in
// structure.ipp.
RawColorRenderImageQuantity* createRawColorRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                       const std::vector<float>& depthData,
                                                       const std::vector<glm::vec3>& colorData,
                                                       ImageOrigin imageOrigin) {
  return new RawColorRenderImageQuantity(parent, name, dimX, dimY, depthData, colorData, imageOrigin);
}

} // namespace polyscope
