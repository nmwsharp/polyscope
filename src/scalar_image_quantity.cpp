// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/polyscope.h"

#include "polyscope/scalar_image_quantity.h"

#include "imgui.h"

namespace polyscope {


ScalarImageQuantity::ScalarImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                         const std::vector<double>& data_, ImageOrigin imageOrigin_, DataType dataType_)
    : ImageQuantity(parent_, name, dimX, dimY, imageOrigin_), ScalarQuantity(*this, data_, dataType_) {}


void ScalarImageQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    buildScalarOptionsUI();
    buildImageOptionsUI();

    ImGui::EndPopup();
  }

  buildScalarUI();
  buildImageUI();
}

void ScalarImageQuantity::ensureRawTexturePopulated() {
  if (textureRaw) return; // already populated, nothing to do

  // Must be rendering from a buffer of data, copy it over (common case)

  values.ensureHostBufferPopulated();
  const std::vector<double>& srcData = values.data;
  std::vector<float> srcDataFloat(srcData.size());
  for (size_t i = 0; i < srcData.size(); i++) {
    srcDataFloat[i] = static_cast<float>(srcData[i]);
  }
  textureRaw = render::engine->generateTextureBuffer(TextureFormat::R32F, dimX, dimY, &(srcDataFloat.front()));
}

void ScalarImageQuantity::prepareIntermediateRender() {
  // Texture and sourceProgram for rendering in
  framebufferIntermediate = render::engine->generateFrameBuffer(dimX, dimY);
  textureIntermediateRendered = render::engine->generateTextureBuffer(TextureFormat::RGB16F, dimX, dimY);
  framebufferIntermediate->addColorBuffer(textureIntermediateRendered);
  framebufferIntermediate->setViewport(0, 0, dimX, dimY);
}

void ScalarImageQuantity::prepareFullscreen() {

  ensureRawTexturePopulated();

  // Create the sourceProgram
  fullscreenProgram = render::engine->requestShader(
      "SCALAR_TEXTURE_COLORMAP", this->addScalarRules({getImageOriginRule(imageOrigin), "TEXTURE_SET_TRANSPARENCY"}),
      render::ShaderReplacementDefaults::Process);
  fullscreenProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  fullscreenProgram->setTextureFromBuffer("t_scalar", textureRaw.get());
  fullscreenProgram->setTextureFromColormap("t_colormap", this->cMap.get());
}

void ScalarImageQuantity::prepareBillboard() {

  ensureRawTexturePopulated();

  // Create the sourceProgram
  billboardProgram =
      render::engine->requestShader("SCALAR_TEXTURE_COLORMAP",
                                    this->addScalarRules({getImageOriginRule(imageOrigin), "TEXTURE_SET_TRANSPARENCY",
                                                          "TEXTURE_BILLBOARD_FROM_UNIFORMS"}),
                                    render::ShaderReplacementDefaults::Process);
  billboardProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  billboardProgram->setTextureFromBuffer("t_scalar", textureRaw.get());
  billboardProgram->setTextureFromColormap("t_colormap", this->cMap.get());
}

void ScalarImageQuantity::showFullscreen() {

  if (!fullscreenProgram) {
    prepareFullscreen();
  }

  // Set uniforms
  this->setScalarUniforms(*fullscreenProgram);
  fullscreenProgram->setUniform("u_transparency", getTransparency());

  fullscreenProgram->draw();

  render::engine->applyTransparencySettings();
}

void ScalarImageQuantity::renderIntermediate() {
  if (!fullscreenProgram) prepareFullscreen();
  if (!textureIntermediateRendered) prepareIntermediateRender();
  ensureRawTexturePopulated();

  // Set uniforms
  this->setScalarUniforms(*fullscreenProgram);
  fullscreenProgram->setUniform("u_transparency", getTransparency());

  // render to the intermediate texture
  render::engine->pushBindFramebufferForRendering(*framebufferIntermediate);
  fullscreenProgram->draw();
  render::engine->popBindFramebufferForRendering();
  render::engine->applyTransparencySettings();
}

void ScalarImageQuantity::showInImGuiWindow() {
  if (!textureIntermediateRendered) return;

  ImGui::Begin(name.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar);

  float w = ImGui::GetWindowWidth();
  float h = w * dimY / dimX;

  ImGui::Text("Dimensions: %zux%zu", dimX, dimY);

  // here we always use the same ImVec2 UV coords below, because the texture order is always openGL convention after the
  // intermediate render pass
  ImGui::Image(textureIntermediateRendered->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));


  ImGui::End();
}

void ScalarImageQuantity::showInBillboard(glm::vec3 center, glm::vec3 upVec, glm::vec3 rightVec) {

  if (!billboardProgram) {
    prepareBillboard();
  }

  // ensure the scale of rightVec matches the aspect ratio of the image
  rightVec = glm::normalize(rightVec) * glm::length(upVec) * ((float)dimX / dimY);

  // set uniforms
  parent.setStructureUniforms(*billboardProgram);
  billboardProgram->setUniform("u_transparency", getTransparency());
  billboardProgram->setUniform("u_billboardCenter", center);
  billboardProgram->setUniform("u_billboardUp", upVec);
  billboardProgram->setUniform("u_billboardRight", rightVec);
  this->setScalarUniforms(*billboardProgram);

  render::engine->setBackfaceCull(false);
  billboardProgram->draw();
  render::engine->setBackfaceCull(); // return to default setting
}

void ScalarImageQuantity::refresh() {
  fullscreenProgram.reset();
  billboardProgram.reset();
  Quantity::refresh();
}

std::string ScalarImageQuantity::niceName() { return name + " (scalar image)"; }

ScalarImageQuantity* ScalarImageQuantity::setEnabled(bool newEnabled) {
  if (newEnabled == isEnabled()) return this;
  if (newEnabled == true && getShowFullscreen()) {
    // if drawing fullscreen, disable anything else which was already drawing fullscreen
    disableAllFullscreenArtists();
  }
  enabled = newEnabled;
  requestRedraw();
  return this;
}


// Instantiate a construction helper which is used to avoid header dependencies. See forward declaration and note in
// structure.ipp.
ScalarImageQuantity* createScalarImageQuantity(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                               const std::vector<double>& data, ImageOrigin imageOrigin,
                                               DataType dataType) {
  return new ScalarImageQuantity(parent, name, dimX, dimY, data, imageOrigin, dataType);
}

} // namespace polyscope
