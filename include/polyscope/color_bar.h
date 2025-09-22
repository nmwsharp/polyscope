// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/color_maps.h"
#include "polyscope/render/engine.h"

#include <vector>


namespace polyscope {

// A histogram that shows up in ImGUI window
// ONEDAY: we could definitely make a better histogram widget for categorical data...

class ColorBar {
public:
  ColorBar();                                              // must call buildHistogram() with data after
  ColorBar(std::vector<float>& values, DataType datatype); // internally calls buildHistogram()

  ~ColorBar();

  void buildHistogram(const std::vector<float>& values, DataType datatype);
  void updateColormap(const std::string& newColormap);

  // Width = -1 means set automatically
  void buildUI(float width = -1.0);

  std::pair<double, double> colormapRange; // in DATA values, not [0,1]

private:
  //
  DataType dataType = DataType::STANDARD;
  std::pair<double, double> dataRange;

  // == The inline horizontal histogram visualization in the structures bar

  // Manage histogram counts for 
  void fillHistogramBuffers();
  size_t rawHistBinCount = 51;
  std::vector<float> rawHistCurveY;
  std::vector<std::array<float, 2>> rawHistCurveX;

  // Render to a texture for the inline histogram visualization in the structures bar
  void renderInlineHistogramToTexture();
  void prepareInlineHistogram();
  unsigned int texDim = 600;
  std::shared_ptr<render::TextureBuffer> inlineHistogramTexture = nullptr;
  std::shared_ptr<render::FrameBuffer> inlineHistogramFramebuffer = nullptr;
  std::shared_ptr<render::ShaderProgram> inlineHistogramProgram = nullptr;
  std::string colormap = "viridis";

  // A few parameters which control appearance
  float bottomBarHeight = 0.35;
  float bottomBarGap = 0.1;
  
  // == The optional vertical colorbar which floats ont he main display
  
};


}; // namespace polyscope
