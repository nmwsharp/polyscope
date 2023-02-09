// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/polyscope.h"

#include "polyscope/image_color_artist.h"
#include "polyscope/render/engine.h"
#include "polyscope/types.h"

#include <vector>

namespace polyscope {

ImageColorArtist::ImageColorArtist(std::string displayName_, std::string uniquePrefix_, size_t dimX_, size_t dimY_,
                                   const std::vector<glm::vec4>& data_, ImageOrigin imageOrigin_)
    : displayName(displayName_), dimX(dimX_), dimY(dimY_), data(data_),
      transparency(uniquePrefix_ + "#" + displayName_, 1.0f), imageOrigin(imageOrigin_) {}

void ImageColorArtist::ensureRawTexturePopulated() {
  if (textureRaw) return; // already populated, nothing to do

  if (readFromTex) {
    // sanity check for the special case of rendering an existing buffer
    throw std::runtime_error("image artist should be rendering from texture, but texture is null");
  }

  // Must be rendering from a buffer of data, copy it over (common case)

  textureRaw = render::engine->generateTextureBuffer(TextureFormat::RGBA32F, dimX, dimY, &(data.front()[0]));
}


void ImageColorArtist::prepareFullscreen() {

  ensureRawTexturePopulated();

  // Create the sourceProgram
  fullscreenProgram =
      render::engine->requestShader("TEXTURE_DRAW_PLAIN", {getImageOriginRule(imageOrigin), "TEXTURE_SET_TRANSPARENCY"},
                                    render::ShaderReplacementDefaults::Process);
  fullscreenProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  fullscreenProgram->setTextureFromBuffer("t_image", textureRaw.get());
}

void ImageColorArtist::prepareBillboard() {

  ensureRawTexturePopulated();

  // Create the sourceProgram
  billboardProgram = render::engine->requestShader(
      "TEXTURE_DRAW_PLAIN",
      {getImageOriginRule(imageOrigin), "TEXTURE_SET_TRANSPARENCY", "TEXTURE_BILLBOARD_FROM_UNIFORMS"},
      render::ShaderReplacementDefaults::Process);
  billboardProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  billboardProgram->setTextureFromBuffer("t_image", textureRaw.get());
}


void ImageColorArtist::refresh() {
  fullscreenProgram.reset();
  billboardProgram.reset();
}


void ImageColorArtist::showFullscreen() {

  if (!fullscreenProgram) {
    prepareFullscreen();
  }

  // Set uniforms
  fullscreenProgram->setUniform("u_transparency", getTransparency());

  fullscreenProgram->draw();

  render::engine->applyTransparencySettings();
}


void ImageColorArtist::showInImGuiWindow() {
  if (!fullscreenProgram) prepareFullscreen();
  ensureRawTexturePopulated();

  ImGui::Begin(displayName.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar);

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

void ImageColorArtist::showInBillboard(glm::vec3 center, glm::vec3 upVec, glm::vec3 rightVec) {

  if (!billboardProgram) {
    prepareBillboard();
  }

  // ensure the scale of rightVec matches the aspect ratio of the image
  rightVec = glm::normalize(rightVec) * glm::length(upVec) * ((float)dimX / dimY);

  // set uniforms
  billboardProgram->setUniform("u_transparency", getTransparency());
  billboardProgram->setUniform("u_billboardCenter", center);
  billboardProgram->setUniform("u_billboardUp", upVec);
  billboardProgram->setUniform("u_billboardRight", rightVec);

  billboardProgram->draw();
}

void ImageColorArtist::setTransparency(float newVal) {
  transparency = newVal;
  requestRedraw();
}

float ImageColorArtist::getTransparency() { return transparency.get(); }

} // namespace polyscope
