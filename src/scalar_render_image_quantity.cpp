// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/polyscope.h"

#include "polyscope/scalar_render_image_quantity.h"

#include "imgui.h"

namespace polyscope {


ScalarRenderImageQuantity::ScalarRenderImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                                     const std::vector<float>& depthData,
                                                     const std::vector<glm::vec3>& normalData,
                                                     const std::vector<double>& scalarData_, ImageOrigin imageOrigin,
                                                     DataType dataType_)
    : RenderImageQuantityBase(parent_, name, dimX, dimY, depthData, normalData, imageOrigin),
      ScalarQuantity(*this, scalarData_, dataType_) {}

void ScalarRenderImageQuantity::draw() {}

void ScalarRenderImageQuantity::drawDelayed() {
  if (!isEnabled()) return;

  if (!program) prepare();

  // set uniforms
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);

  program->setUniform("u_projMatrix", glm::value_ptr(P));
  program->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  program->setUniform("u_viewport", render::engine->getCurrentViewport());
  program->setUniform("u_transparency", transparency.get());

  setScalarUniforms(*program);

  // make sure we have actual depth testing enabled
  render::engine->setDepthMode(DepthMode::LEqual);
  render::engine->setBlendMode(BlendMode::Over);

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
  textureScalar = nullptr;
  RenderImageQuantityBase::refresh();
}


void ScalarRenderImageQuantity::prepare() {
  prepareGeometryBuffers();

  // push the color data to the buffer
  values.ensureHostBufferPopulated();
  std::vector<float> floatData(values.data.size());
  for (size_t i = 0; i < values.data.size(); i++) {
    floatData[i] = static_cast<float>(values.data[i]);
  }

  textureScalar =
      render::engine->generateTextureBuffer(TextureFormat::R32F, dimX, dimY, static_cast<float*>(&floatData.front()));

  // Create the sourceProgram
  program = render::engine->requestShader("TEXTURE_DRAW_RENDERIMAGE_PLAIN",
                                          addScalarRules({getImageOriginRule(imageOrigin), "LIGHT_MATCAP",
                                                          "TEXTURE_PROPAGATE_VALUE", "SHADE_COLORMAP_VALUE"}),
                                          render::ShaderReplacementDefaults::Process);

  program->setAttribute("a_position", render::engine->screenTrianglesCoords());
  program->setTextureFromBuffer("t_depth", textureDepth.get());
  program->setTextureFromBuffer("t_normal", textureNormal.get());
  program->setTextureFromBuffer("t_scalar", textureScalar.get());
  render::engine->setMaterial(*program, material.get());
  program->setTextureFromColormap("t_colormap", cMap.get());
}


std::string ScalarRenderImageQuantity::niceName() { return name + " (scalar render image)"; }

ScalarRenderImageQuantity* ScalarRenderImageQuantity::setEnabled(bool newEnabled) {
  enabled = newEnabled;
  requestRedraw();
  return this;
}


// Instantiate a construction helper which is used to avoid header dependencies. See forward declaration and note in
// structure.ipp.
ScalarRenderImageQuantity* createScalarRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                   const std::vector<float>& depthData,
                                                   const std::vector<glm::vec3>& normalData,
                                                   const std::vector<double>& scalarData, ImageOrigin imageOrigin,
                                                   DataType dataType) {

  return new ScalarRenderImageQuantity(parent, name, dimX, dimY, depthData, normalData, scalarData, imageOrigin,
                                       dataType);
}

} // namespace polyscope
