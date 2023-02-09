// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once
#include "polyscope/image_scalar_artist.h"

#include "polyscope/polyscope.h"

#include <vector>

namespace polyscope {

template <typename QuantityT>
ImageScalarArtist<QuantityT>::ImageScalarArtist(QuantityT& parentQ_, std::string displayName_, size_t dimX_,
                                                size_t dimY_, const std::vector<double>& data_,
                                                ImageOrigin imageOrigin_, DataType dataType_)
    : ScalarQuantity<QuantityT>(parentQ_, data_, dataType_), displayName(displayName_), dimX(dimX_), dimY(dimY_),
      transparency(parentQ_.uniquePrefix() + "#" + displayName_, 1.0f), imageOrigin(imageOrigin_) {}

template <typename QuantityT>
void ImageScalarArtist<QuantityT>::ensureRawTexturePopulated() {
  if (textureRaw) return; // already populated, nothing to do

  if (readFromTex) {
    // sanity check for the special case of rendering an existing buffer
    throw std::runtime_error("image artist should be rendering from texture, but texture is null");
  }

  // Must be rendering from a buffer of data, copy it over (common case)

  ScalarQuantity<QuantityT>::values.ensureHostBufferPopulated();
  const std::vector<double>& srcData = ScalarQuantity<QuantityT>::values.data;
  std::vector<float> srcDataFloat(srcData.size());
  for (size_t i = 0; i < srcData.size(); i++) {
    srcDataFloat[i] = static_cast<float>(srcData[i]);
  }
  textureRaw = render::engine->generateTextureBuffer(TextureFormat::R32F, dimX, dimY, &(srcDataFloat.front()));
}

// template <typename QuantityT>
// void ImageScalarArtist<QuantityT>::prepareSource() {
//   // Fill a texture with the raw data
//   if (!readFromTex) {
//     // copy to float
//     ScalarQuantity<QuantityT>::values.ensureHostBufferPopulated();
//     const std::vector<double>& src = ScalarQuantity<QuantityT>::values.data;
//     std::vector<float> floatValues(src.size());
//     for (size_t i = 0; i < src.size(); i++) {
//       floatValues[i] = static_cast<float>(src[i]);
//     }
//
//     // common case
//     textureRaw = render::engine->generateTextureBuffer(TextureFormat::R32F, dimX, dimY, &floatValues.front());
//   }
//
//   // Texture and sourceProgram for rendering in
//   framebuffer = render::engine->generateFrameBuffer(dimX, dimY);
//   textureRendered = render::engine->generateTextureBuffer(TextureFormat::RGB16F, dimX, dimY);
//   framebuffer->addColorBuffer(textureRendered);
//   framebuffer->setViewport(0, 0, dimX, dimY);
// }

template <typename QuantityT>
void ImageScalarArtist<QuantityT>::prepareIntermediateRender() {
  // Texture and sourceProgram for rendering in
  framebufferIntermediate = render::engine->generateFrameBuffer(dimX, dimY);
  textureIntermediateRendered = render::engine->generateTextureBuffer(TextureFormat::RGB16F, dimX, dimY);
  framebufferIntermediate->addColorBuffer(textureIntermediateRendered);
  framebufferIntermediate->setViewport(0, 0, dimX, dimY);
}

template <typename QuantityT>
void ImageScalarArtist<QuantityT>::prepareFullscreen() {

  ensureRawTexturePopulated();

  // Create the sourceProgram
  fullscreenProgram = render::engine->requestShader(
      "SCALAR_TEXTURE_COLORMAP", this->addScalarRules({getImageOriginRule(imageOrigin), "TEXTURE_SET_TRANSPARENCY"}),
      render::ShaderReplacementDefaults::Process);
  fullscreenProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  fullscreenProgram->setTextureFromBuffer("t_scalar", textureRaw.get());
  fullscreenProgram->setTextureFromColormap("t_colormap", this->cMap.get());
}

template <typename QuantityT>
void ImageScalarArtist<QuantityT>::prepareBillboard() {

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

// template <typename QuantityT>
// void ImageScalarArtist<QuantityT>::prepare() {
//   if (textureRaw == nullptr) {
//     // the first time, we need to also allocate the buffers for the raw source data
//     prepareSource();
//   }
//
//   // Create the sourceProgram
//   sourceProgram = render::engine->requestShader(
//       "SCALAR_TEXTURE_COLORMAP", this->addScalarRules({"TEXTURE_ORIGIN_UPPERLEFT", "TEXTURE_SET_TRANSPARENCY"}),
//       render::ShaderReplacementDefaults::Process);
//   sourceProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
//   sourceProgram->setTextureFromBuffer("t_scalar", textureRaw.get());
//   sourceProgram->setTextureFromColormap("t_colormap", this->cMap.get());
// }


template <typename QuantityT>
void ImageScalarArtist<QuantityT>::refresh() {
  fullscreenProgram.reset();
  billboardProgram.reset();
}

// template <typename QuantityT>
// void ImageScalarArtist<QuantityT>::renderSource() {
//   // === Render the raw data to the texture
//
//   // Make the sourceProgram if we don't have one already
//   if (sourceProgram == nullptr) {
//     prepare();
//   }
//
//   // Set uniforms
//   this->setScalarUniforms(*sourceProgram);
//
//   // Render to the intermediate "source" texture for the image
//   render::engine->pushBindFramebufferForRendering(*framebuffer);
//   sourceProgram->setUniform("u_transparency", 1.0f);
//   sourceProgram->draw();
//   render::engine->popBindFramebufferForRendering();
// }


template <typename QuantityT>
void ImageScalarArtist<QuantityT>::showFullscreen() {

  if (!fullscreenProgram) {
    prepareFullscreen();
  }

  // Set uniforms
  this->setScalarUniforms(*fullscreenProgram);
  fullscreenProgram->setUniform("u_transparency", getTransparency());

  fullscreenProgram->draw();

  render::engine->applyTransparencySettings();
}

template <typename QuantityT>
void ImageScalarArtist<QuantityT>::renderIntermediate() {
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

template <typename QuantityT>
void ImageScalarArtist<QuantityT>::showInImGuiWindow() {
  if (!textureIntermediateRendered) return;

  ImGui::Begin(displayName.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar);

  float w = ImGui::GetWindowWidth();
  float h = w * dimY / dimX;

  ImGui::Text("Dimensions: %zux%zu", dimX, dimY);

  // here we always use the same ImVec2 UV coords below, because the texture order is always openGL convention after the
  // intermediate render pass
  ImGui::Image(textureIntermediateRendered->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));


  ImGui::End();
}

template <typename QuantityT>
void ImageScalarArtist<QuantityT>::setTransparency(float newVal) {
  transparency = newVal;
  requestRedraw();
}

template <typename QuantityT>
float ImageScalarArtist<QuantityT>::getTransparency() {
  return transparency.get();
}

} // namespace polyscope
