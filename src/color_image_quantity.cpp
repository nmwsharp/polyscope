// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/polyscope.h"

#include "polyscope/color_image_quantity.h"

#include "imgui.h"
#include "polyscope/render/engine.h"

namespace polyscope {


ColorImageQuantity::ColorImageQuantity(Structure& parent_, std::string name, size_t dimX, size_t dimY,
                                       const std::vector<glm::vec4>& data_, ImageOrigin imageOrigin_)
    : ImageQuantity(parent_, name, dimX, dimY, imageOrigin_), data(data_) {}


void ColorImageQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    buildImageOptionsUI();

    ImGui::EndPopup();
  }

  buildImageUI();
}

std::string ColorImageQuantity::niceName() { return name + " (color image)"; }

void ColorImageQuantity::ensureRawTexturePopulated() {
  if (textureRaw) return; // already populated, nothing to do

  // Must be rendering from a buffer of data, copy it over (common case)

  textureRaw = render::engine->generateTextureBuffer(TextureFormat::RGBA32F, dimX, dimY, &(data.front()[0]));
}

void ColorImageQuantity::prepareFullscreen() {

  ensureRawTexturePopulated();

  // Create the sourceProgram
  fullscreenProgram =
      render::engine->requestShader("TEXTURE_DRAW_PLAIN", {getImageOriginRule(imageOrigin), "TEXTURE_SET_TRANSPARENCY"},
                                    render::ShaderReplacementDefaults::Process);
  fullscreenProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  fullscreenProgram->setTextureFromBuffer("t_image", textureRaw.get());
}

void ColorImageQuantity::prepareBillboard() {

  ensureRawTexturePopulated();

  // Create the sourceProgram
  billboardProgram = render::engine->requestShader(
      "TEXTURE_DRAW_PLAIN",
      {getImageOriginRule(imageOrigin), "TEXTURE_SET_TRANSPARENCY", "TEXTURE_BILLBOARD_FROM_UNIFORMS"},
      render::ShaderReplacementDefaults::Process);
  billboardProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  billboardProgram->setTextureFromBuffer("t_image", textureRaw.get());
}

void ColorImageQuantity::showFullscreen() {

  if (!fullscreenProgram) {
    prepareFullscreen();
  }

  // Set uniforms
  fullscreenProgram->setUniform("u_transparency", getTransparency());

  fullscreenProgram->draw();

  render::engine->applyTransparencySettings();
}


void ColorImageQuantity::showInImGuiWindow() {
  if (!fullscreenProgram) prepareFullscreen();
  ensureRawTexturePopulated();

  ImGui::Begin(name.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar);

  float w = ImGui::GetWindowWidth();
  float h = w * dimY / dimX;

  ImGui::Text("Dimensions: %zux%zu", dimX, dimY);

  // since we are showing directly from the user's texture, we need to resposect the upper left ordering
  if (imageOrigin == ImageOrigin::LowerLeft) {
    ImGui::Image(textureRaw->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));
  } else if (imageOrigin == ImageOrigin::UpperLeft) {
    ImGui::Image(textureRaw->getNativeHandle(), ImVec2(w, h));
  }

  ImGui::End();
}

void ColorImageQuantity::showInBillboard(glm::vec3 center, glm::vec3 upVec, glm::vec3 rightVec) {

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

  render::engine->setBackfaceCull(false);
  render::engine->setBlendMode(
      BlendMode::AlphaOver); // WARNING: I never really thought through this, may cause problems
  billboardProgram->draw();
  render::engine->setBackfaceCull(); // return to default setting
  render::engine->applyTransparencySettings();
}


void ColorImageQuantity::refresh() {
  fullscreenProgram.reset();
  billboardProgram.reset();
  Quantity::refresh();
}


ColorImageQuantity* ColorImageQuantity::setEnabled(bool newEnabled) {
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
ColorImageQuantity* createColorImageQuantity(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                             const std::vector<glm::vec4>& data, ImageOrigin imageOrigin) {
  return new ColorImageQuantity(parent, name, dimX, dimY, data, imageOrigin);
}


} // namespace polyscope
