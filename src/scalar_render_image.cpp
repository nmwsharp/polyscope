// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/polyscope.h"

#include "polyscope/scalar_render_image.h"

#include "imgui.h"

namespace polyscope {


ScalarRenderImage::ScalarRenderImage(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                     const std::vector<float>& depthData, const std::vector<glm::vec3>& normalData,
                                     const std::vector<double>& scalarData_, DataType dataType_)
    : RenderImageQuantityBase(parent_, name, dimX, dimY, depthData, normalData),
      ScalarQuantity(*this, scalarData_, dataType_) {}

void ScalarRenderImage::draw() {}

void ScalarRenderImage::drawDelayed() {
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

void ScalarRenderImage::buildCustomUI() {
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


void ScalarRenderImage::refresh() {
  program = nullptr;
  textureScalar = nullptr;
  RenderImageQuantityBase::refresh();
}


void ScalarRenderImage::prepare() {
  prepareGeometryBuffers();

  // push the color data to the buffer
  std::vector<float> floatData(values.size());
  for (size_t i = 0; i < values.size(); i++) {
    floatData[i] = static_cast<float>(values[i]);
  }

  textureScalar =
      render::engine->generateTextureBuffer(TextureFormat::R32F, dimX, dimY, static_cast<float*>(&floatData.front()));

  // Create the sourceProgram
  program = render::engine->requestShader(
      "TEXTURE_DRAW_RENDERIMAGE_PLAIN",
      addScalarRules({"TEXTURE_ORIGIN_UPPERLEFT", "LIGHT_MATCAP", "TEXTURE_PROPAGATE_VALUE", "SHADE_COLORMAP_VALUE"}),
      render::ShaderReplacementDefaults::Process);

  program->setAttribute("a_position", render::engine->screenTrianglesCoords());
  program->setTextureFromBuffer("t_depth", textureDepth.get());
  program->setTextureFromBuffer("t_normal", textureNormal.get());
  program->setTextureFromBuffer("t_scalar", textureScalar.get());
  render::engine->setMaterial(*program, material.get());
  program->setTextureFromColormap("t_colormap", cMap.get());
}


std::string ScalarRenderImage::niceName() { return name + " (scalar render image)"; }

ScalarRenderImage* ScalarRenderImage::setEnabled(bool newEnabled) {
  enabled = newEnabled;
  requestRedraw();
  return this;
}


// Instantiate a construction helper which is used to avoid header dependencies. See forward declaration and note in
// structure.ipp.
ScalarRenderImage* createScalarRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                           const std::vector<float>& depthData,
                                           const std::vector<glm::vec3>& normalData,
                                           const std::vector<double>& scalarData, DataType dataType) {

  return new ScalarRenderImage(parent, name, dimX, dimY, depthData, normalData, scalarData, dataType);
}

} // namespace polyscope
