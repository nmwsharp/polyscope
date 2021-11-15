// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/image_color_artist.h"

#include "polyscope/polyscope.h"

#include <vector>

namespace polyscope {

ImageColorArtist::ImageColorArtist(std::string displayName_, size_t dimX_, size_t dimY_,
                                   const std::vector<glm::vec4>& data_)
    : displayName(displayName_), dimX(dimX_), dimY(dimY_), data(data_) {}

/*
ImageColorArtist::ImageColorArtist(std::string name_,
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


void ImageColorArtist::prepareSource() {
  // Fill a texture with the raw data
  if (!readFromTex) {
    // copy to flat float buffer, just to be safe
    std::vector<float> floatValues(4 * data.size());
    for (size_t i = 0; i < data.size(); i++) {
      for (size_t j = 0; j < 4; j++) {
        floatValues[4 * i + j] = static_cast<float>(data[i][j]);
      }
    }

    // common case
    textureRaw = render::engine->generateTextureBuffer(TextureFormat::RGBA32F, dimX, dimY, &floatValues.front());
  }

  // Texture and sourceProgram for rendering in
  framebuffer = render::engine->generateFrameBuffer(dimX, dimY);
  textureRendered = render::engine->generateTextureBuffer(TextureFormat::RGBA16F, dimX, dimY);
  framebuffer->addColorBuffer(textureRendered);
  framebuffer->setViewport(0, 0, dimX, dimY);
}

void ImageColorArtist::prepare() {
  if (textureRaw == nullptr) {
    // the first time, we need to also allocate the buffers for the raw source data
    prepareSource();
  }

  // Create the sourceProgram
  sourceProgram = render::engine->requestShader("TEXTURE_DRAW_PLAIN", {}, render::ShaderReplacementDefaults::Process);
  sourceProgram->setAttribute("a_position", render::engine->screenTrianglesCoords());
  sourceProgram->setTextureFromBuffer("t_image", textureRaw.get());
}


void ImageColorArtist::refresh() {
  sourceProgram.reset();
  fullscreenProgram.reset();
  textureRendered.reset();
  framebuffer.reset();
  if (!readFromTex) {
    // TODO will this actually work out okay in the "view texture" case?
    textureRaw.reset();
  }
}

void ImageColorArtist::renderSource() {
  // === Render the raw data to the texture

  // Make the sourceProgram if we don't have one already
  if (sourceProgram == nullptr) {
    prepare();
  }

  // Set uniforms

  // Render to the intermediate "source" texture for the image
  render::engine->pushBindFramebufferForRendering(*framebuffer);
  sourceProgram->draw();
  render::engine->popBindFramebufferForRendering();
}


void ImageColorArtist::showFullscreen() {
  // === Render the raw data to the texture

  // Make the sourceProgram if we don't have one already
  if (sourceProgram == nullptr) {
    // prepareFullscreen();
    prepare();
  }

  // render::engine->pushBindFramebufferForRendering(*framebuffer);
  sourceProgram->draw();
  // render::engine->popBindFramebufferForRendering();
}


void ImageColorArtist::showInImGuiWindow() {
  if (!sourceProgram) return;

  ImGui::Begin(displayName.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar);

  float w = ImGui::GetWindowWidth();
  float h = w * dimY / dimX;

  ImGui::Text("Dimensions: %zux%zu", dimX, dimY);
  ImGui::Image(textureRendered->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));


  ImGui::End();
}


} // namespace polyscope
