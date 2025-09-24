// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/polyscope.h"

#include "polyscope/scalar_render_image_quantity.h"

#include "imgui.h"

namespace polyscope {


ScalarRenderImageQuantity::ScalarRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                                     const std::vector<float>& depthData,
                                                     const std::vector<glm::vec3>& normalData,
                                                     const std::vector<float>& scalarData_, ImageOrigin imageOrigin,
                                                     DataType dataType_)
    : RenderImageQuantityBase(parent_, name, dimX, dimY, depthData, normalData, imageOrigin),
      ScalarQuantity(*this, scalarData_, dataType_) {
  values.setTextureSize(dimX, dimY);
}

void ScalarRenderImageQuantity::draw() {}

void ScalarRenderImageQuantity::drawDelayed() {
  if (!isEnabled()) return;

  if (!program) prepare();

  setRenderImageUniforms(*program);
  setScalarUniforms(*program);
  render::engine->setMaterialUniforms(*program, material.get());

  // draw
  program->draw();
}

void ScalarRenderImageQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    RenderImageQuantityBase::addOptionsPopupEntries();

    buildScalarOptionsUI();

    ImGui::EndPopup();
  }

  buildScalarUI();
}


void ScalarRenderImageQuantity::refresh() {
  program = nullptr;
  RenderImageQuantityBase::refresh();
}


void ScalarRenderImageQuantity::prepare() {

  // push the color data to the buffer
  values.ensureHostBufferPopulated();
  std::vector<float> floatData(values.data.size());
  for (size_t i = 0; i < values.data.size(); i++) {
    floatData[i] = static_cast<float>(values.data[i]);
  }

  // Create the sourceProgram
  // clang-format off
  program = render::engine->requestShader("TEXTURE_DRAW_RENDERIMAGE_PLAIN",
    render::engine->addMaterialRules(material.get(),
      addScalarRules(
        parent.addStructureRules({
          getImageOriginRule(imageOrigin), 
          hasNormals ? "SHADE_NORMAL_FROM_TEXTURE" : "SHADE_NORMAL_FROM_VIEWPOS_VAR",
          "TEXTURE_PROPAGATE_VALUE", 
        })
      )
    ),
    render::ShaderReplacementDefaults::SceneObjectNoSlice);
  // clang-format on

  program->setAttribute("a_position", render::engine->screenTrianglesCoords());
  program->setTextureFromBuffer("t_depth", depths.getRenderTextureBuffer().get());
  if (hasNormals) {
    program->setTextureFromBuffer("t_normal", normals.getRenderTextureBuffer().get());
  }
  program->setTextureFromBuffer("t_scalar", values.getRenderTextureBuffer().get());
  render::engine->setMaterial(*program, material.get());
  program->setTextureFromColormap("t_colormap", cMap.get());
}


std::string ScalarRenderImageQuantity::niceName() { return name + " (scalar render image)"; }


// Instantiate a construction helper which is used to avoid header dependencies. See forward declaration and note in
// structure.ipp.
ScalarRenderImageQuantity* createScalarRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                   const std::vector<float>& depthData,
                                                   const std::vector<glm::vec3>& normalData,
                                                   const std::vector<float>& scalarData, ImageOrigin imageOrigin,
                                                   DataType dataType) {

  return new ScalarRenderImageQuantity(parent, name, dimX, dimY, depthData, normalData, scalarData, imageOrigin,
                                       dataType);
}

} // namespace polyscope
