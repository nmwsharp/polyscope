// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/polyscope.h"

#include "polyscope/color_render_image_quantity.h"

#include "imgui.h"
#include "polyscope/render/engine.h"

namespace polyscope {


ColorRenderImageQuantity::ColorRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                                   const std::vector<float>& depthData,
                                                   const std::vector<glm::vec3>& normalData,
                                                   const std::vector<glm::vec3>& colorsData_, ImageOrigin imageOrigin)
    : RenderImageQuantityBase(parent_, name, dimX, dimY, depthData, normalData, imageOrigin),
      colors(this, uniquePrefix() + "colors", colorsData), colorsData(colorsData_) {
  colors.setTextureSize(dimX, dimY);
}

void ColorRenderImageQuantity::draw() {}

void ColorRenderImageQuantity::drawDelayed() {
  if (!isEnabled()) return;

  if (!program) prepare();

  setRenderImageUniforms(*program);

  // draw
  program->draw();
}

void ColorRenderImageQuantity::buildCustomUI() {
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


void ColorRenderImageQuantity::refresh() {
  program = nullptr;
  RenderImageQuantityBase::refresh();
}


void ColorRenderImageQuantity::prepare() {

  // Create the sourceProgram
  // clang-format off
  program = render::engine->requestShader("TEXTURE_DRAW_RENDERIMAGE_PLAIN",
    render::engine->addMaterialRules(material.get(),
      parent.addStructureRules({
        getImageOriginRule(imageOrigin), 
        hasNormals ? "SHADE_NORMAL_FROM_TEXTURE" : "SHADE_NORMAL_FROM_VIEWPOS_VAR",
        "TEXTURE_SHADE_COLOR"
      })
    ), 
    render::ShaderReplacementDefaults::SceneObjectNoSlice);
  // clang-format on

  program->setAttribute("a_position", render::engine->screenTrianglesCoords());
  program->setTextureFromBuffer("t_depth", depths.getRenderTextureBuffer().get());
  if (hasNormals) {
    program->setTextureFromBuffer("t_normal", normals.getRenderTextureBuffer().get());
  }
  program->setTextureFromBuffer("t_color", colors.getRenderTextureBuffer().get());
  render::engine->setMaterial(*program, material.get());
}


std::string ColorRenderImageQuantity::niceName() { return name + " (color render image)"; }

// Instantiate a construction helper which is used to avoid header dependencies. See forward declaration and note in
// structure.ipp.
ColorRenderImageQuantity* createColorRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                 const std::vector<float>& depthData,
                                                 const std::vector<glm::vec3>& normalData,
                                                 const std::vector<glm::vec3>& colorData, ImageOrigin imageOrigin) {
  return new ColorRenderImageQuantity(parent, name, dimX, dimY, depthData, normalData, colorData, imageOrigin);
}

} // namespace polyscope
