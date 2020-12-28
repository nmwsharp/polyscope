// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/persistent_value.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/render/engine.h"
#include "polyscope/scaled_value.h"

#include <vector>


namespace polyscope {

class ImageScalarArtist {

public:
  // ImageScalarArtist(, DataType dataType);
  ImageScalarArtist(std::string name, const std::vector<float>& data, size_t dimX, size_t dimY,
                    DataType dataType = DataType::STANDARD);

  // An alternate constructor which bypasses the float array and just reads directly from the texture. Limits will be
  // set arbitrarily. This is a bit of a hack, and mainly used for visualizing internal rendering buffers.
  ImageScalarArtist(std::string name, std::shared_ptr<render::TextureBuffer>& texturebuffer, size_t dimX, size_t dimY,
                    DataType dataType = DataType::STANDARD);

  void draw(); // (re-)render the data to the internal texture

  void buildImGUIWindow(); // build a floating imgui window showing the texture

  const std::string name;
  std::vector<float> data;
  const size_t dimX, dimY;
  const DataType dataType;
  const bool readFromTex = false; // hack to also support pulling directly from a texture


  // === Get/set visualization parameters

  // The color map
  ImageScalarArtist* setColorMap(std::string val);
  std::string getColorMap();

  // Data limits mapped in to colormap
  ImageScalarArtist* setMapRange(std::pair<double, double> val);
  std::pair<double, double> getMapRange();
  ImageScalarArtist* resetMapRange(); // reset to full range

private:
  // Affine data maps and limits
  std::pair<float, float> vizRange;
  std::pair<double, double> dataRange;

  // UI internals
  PersistentValue<std::string> cMap;
  std::shared_ptr<render::TextureBuffer> textureRaw, textureRendered;
  std::shared_ptr<render::FrameBuffer> framebuffer;
  std::shared_ptr<render::ShaderProgram> program;

  void prepare();
  void prepareSource();
};

} // namespace polyscope
