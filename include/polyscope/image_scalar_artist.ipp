// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once
#include "polyscope/image_scalar_artist.h"

#include "polyscope/polyscope.h"

#include <vector>

namespace polyscope {

template <typename QuantityT>
ImageScalarArtist<QuantityT>::ImageScalarArtist(QuantityT& parentQ_, std::string displayName_, size_t dimX_,
                                                size_t dimY_, const std::vector<double>& data_, DataType dataType_)
    : ScalarQuantity<QuantityT>(parentQ_, data_, dataType_), displayName(displayName_), dimX(dimX_), dimY(dimY_) {}

/*
template <typename QuantityT>
ImageScalarArtist<QuantityT>::ImageScalarArtist(std::string name_,
                                                std::shared_ptr<render::TextureBuffer>& texturebuffer, size_t dimX_,
                                                size_t dimY_, DataType dataType_)

    : name(name_), dimX(dimX_), dimY(dimY_), dataType(dataType_), readFromTex(true),
      cMap(name + "#cmap", defaultColorMap(dataType)) {

  textureRaw = texturebuffer;
  dataRange.first = 0.;
  dataRange.second = 1.;
  resetMapRange();
  prepareSource();
}
*/


template <typename QuantityT>
void ImageScalarArtist<QuantityT>::prepareSource() {
  // Fill a texture with the raw data
  if (!readFromTex) {
    // copy to float
    const std::vector<double>& src = ScalarQuantity<QuantityT>::values;
    std::vector<float> floatValues(src.size());
    for (size_t i = 0; i < src.size(); i++) {
      floatValues[i] = static_cast<float>(src[i]);
    }

    // common case
    textureRaw = render::engine->generateTextureBuffer(TextureFormat::R32F, dimX, dimY, &floatValues.front());
  }

  // Texture and sourceProgram for rendering in
  framebuffer = render::engine->generateFrameBuffer(dimX, dimY);
  textureRendered = render::engine->generateTextureBuffer(TextureFormat::RGB16F, dimX, dimY);
  framebuffer->addColorBuffer(textureRendered);
  framebuffer->setViewport(0, 0, dimX, dimY);
}

template <typename QuantityT>
void ImageScalarArtist<QuantityT>::prepare() {
  if (textureRaw == nullptr) {
    // the first time, we need to also allocate the buffers for the raw source data
    prepareSource();
  }

  // Create the sourceProgram
  sourceProgram = render::engine->requestShader("SCALAR_TEXTURE_COLORMAP", this->addScalarRules({}),
                                                render::ShaderReplacementDefaults::Process);
  sourceProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  sourceProgram->setTextureFromBuffer("t_scalar", textureRaw.get());
  sourceProgram->setTextureFromColormap("t_colormap", this->cMap.get());
}


/*
TODO restore this functionality?
template <typename QuantityT>
void ImageScalarArtist<QuantityT>::prepareFullscreen() {

  // Create the sourceProgram
  sourceProgram = render::engine->requestShader("SCALAR_TEXTURE_COLORMAP", {"SHADE_COLORMAP_VALUE"},
                                                render::ShaderReplacementDefaults::Process);
  sourceProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  sourceProgram->setTextureFromBuffer("t_scalar", textureRaw.get());
  sourceProgram->setTextureFromColormap("t_colormap", this->cMap.get());
}
*/

template <typename QuantityT>
void ImageScalarArtist<QuantityT>::refresh() {
  sourceProgram.reset();
  fullscreenProgram.reset();
  textureRendered.reset();
  framebuffer.reset();
  if (!readFromTex) {
    // TODO will this actually work out okay in the "view texture" case?
    textureRaw.reset();
  }
}

template <typename QuantityT>
void ImageScalarArtist<QuantityT>::renderSource() {
  // === Render the raw data to the texture

  // Make the sourceProgram if we don't have one already
  if (sourceProgram == nullptr) {
    prepare();
  }

  // Set uniforms
  this->setScalarUniforms(*sourceProgram);

  // Render to the intermediate "source" texture for the image
  render::engine->pushBindFramebufferForRendering(*framebuffer);
  sourceProgram->draw();
  render::engine->popBindFramebufferForRendering();
}


template <typename QuantityT>
void ImageScalarArtist<QuantityT>::showFullscreen() {
  // === Render the raw data to the texture

  // Make the sourceProgram if we don't have one already
  if (sourceProgram == nullptr) {
    // prepareFullscreen();
    prepare();
  }

  // Set uniforms
  sourceProgram->setUniform("u_rangeLow", this->vizRange.first);
  sourceProgram->setUniform("u_rangeHigh", this->vizRange.second);

  // render::engine->pushBindFramebufferForRendering(*framebuffer);
  sourceProgram->draw();
  // render::engine->popBindFramebufferForRendering();
}


template <typename QuantityT>
void ImageScalarArtist<QuantityT>::showInImGuiWindow() {
  if (!sourceProgram) return;

  ImGui::Begin(displayName.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar);

  float w = ImGui::GetWindowWidth();
  float h = w * dimY / dimX;

  ImGui::Text("Dimensions: %zux%zu", dimX, dimY);
  ImGui::Image(textureRendered->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));


  ImGui::End();
}


} // namespace polyscope
