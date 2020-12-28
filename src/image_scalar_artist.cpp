// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/image_scalar_artist.h"

#include "polyscope/polyscope.h"

#include <vector>

namespace polyscope {

ImageScalarArtist::ImageScalarArtist(std::string name_, const std::vector<float>& data_, size_t dimX_, size_t dimY_,
                                     DataType dataType_)
    : name(name_), data(data_), dimX(dimX_), dimY(dimY_), dataType(dataType_),
      cMap(name + "#cmap", defaultColorMap(dataType)) {
  dataRange = robustMinMax(data, 1e-5);
  // for (int i = 0; i < 10000; i++) {
  // std::cout << "data " << i << " = " << data[i] << "\n";
  //}
  std::cout << "data range: " << dataRange.first << " -- " << dataRange.second << "\n";
  resetMapRange();
}
ImageScalarArtist::ImageScalarArtist(std::string name_, std::shared_ptr<render::TextureBuffer>& texturebuffer,
                                     size_t dimX_, size_t dimY_, DataType dataType_)

    : name(name_), dimX(dimX_), dimY(dimY_), dataType(dataType_), readFromTex(true),
      cMap(name + "#cmap", defaultColorMap(dataType)) {

  textureRaw = texturebuffer;
  dataRange.first = 0.;
  dataRange.second = 1.;
  resetMapRange();
  prepareSource();
}

ImageScalarArtist* ImageScalarArtist::resetMapRange() {
  switch (dataType) {
  case DataType::STANDARD:
    vizRange = dataRange;
    break;
  case DataType::SYMMETRIC: {
    double absRange = std::max(std::abs(dataRange.first), std::abs(dataRange.second));
    vizRange = std::make_pair(-absRange, absRange);
  } break;
  case DataType::MAGNITUDE:
    vizRange = std::make_pair(0., dataRange.second);
    break;
  }

  requestRedraw();
  return this;
}

void ImageScalarArtist::prepareSource() {
  // Fill a texture with the raw data
  if (!readFromTex) {
    // common case
    textureRaw = render::engine->generateTextureBuffer(TextureFormat::R32F, dimX, dimY, &data.front());
  }

  // Texture and program for rendering in
  framebuffer = render::engine->generateFrameBuffer(dimX, dimY);
  textureRendered = render::engine->generateTextureBuffer(TextureFormat::RGB16F, dimX, dimY);
  framebuffer->addColorBuffer(textureRendered);
  framebuffer->setViewport(0, 0, dimX, dimY);
}

void ImageScalarArtist::prepare() {
  if (textureRaw == nullptr) {
    // the first time, we need to also allocate the buffers for the raw source data
    prepareSource();
  }

  // Create the program
  program = render::engine->requestShader("SCALAR_TEXTURE_COLORMAP", {"SHADE_COLORMAP_VALUE"},
                                          render::ShaderReplacementDefaults::Process);
  program->setAttribute("a_position", render::engine->screenTrianglesCoords());
  program->setTextureFromBuffer("t_scalar", textureRaw.get());
  program->setTextureFromColormap("t_colormap", cMap.get());
}

void ImageScalarArtist::draw() {
  // Make the program if we don't have one already
  if (program == nullptr) {
    prepare();
  }

  // Set uniforms
  program->setUniform("u_rangeLow", vizRange.first);
  program->setUniform("u_rangeHigh", vizRange.second);

  framebuffer->bindForRendering();
  program->draw();
}

void ImageScalarArtist::buildImGUIWindow() {
  //draw();

  ImGui::Begin(name.c_str());

  float w = ImGui::GetWindowWidth();
  float h = w * dimY / dimX;

  ImGui::Text("Dimensions: %zux%zu", dimX, dimY);
  ImGui::Image(textureRendered->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

  // Data range
  // Note: %g specifiers are generally nicer than %e, but here we don't acutally have a choice. ImGui (for somewhat
  // valid reasons) links the resolution of the slider to the decimal width of the formatted number. When %g formats a
  // number with few decimal places, sliders can break. There is no way to set a minimum number of decimal places with
  // %g, unfortunately.
  {
    switch (dataType) {
    case DataType::STANDARD:
      ImGui::DragFloatRange2("", &vizRange.first, &vizRange.second, (dataRange.second - dataRange.first) / 100.,
                             dataRange.first, dataRange.second, "Min: %.3e", "Max: %.3e");
      break;
    case DataType::SYMMETRIC: {
      float absRange = std::max(std::abs(dataRange.first), std::abs(dataRange.second));
      ImGui::DragFloatRange2("##range_symmetric", &vizRange.first, &vizRange.second, absRange / 100., -absRange,
                             absRange, "Min: %.3e", "Max: %.3e");
    } break;
    case DataType::MAGNITUDE: {
      ImGui::DragFloatRange2("##range_mag", &vizRange.first, &vizRange.second, vizRange.second / 100., 0.0,
                             dataRange.second, "Min: %.3e", "Max: %.3e");
    } break;
    }
  }


  ImGui::End();
}


ImageScalarArtist* ImageScalarArtist::setColorMap(std::string val) {
  cMap = val;
  program.reset();
  requestRedraw();
  return this;
}
std::string ImageScalarArtist::getColorMap() { return cMap.get(); }

ImageScalarArtist* ImageScalarArtist::setMapRange(std::pair<double, double> val) {
  vizRange = val;
  requestRedraw();
  return this;
}
std::pair<double, double> ImageScalarArtist::getMapRange() { return vizRange; }

} // namespace polyscope
