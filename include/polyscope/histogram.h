// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/color_maps.h"
#include "polyscope/render/engine.h"

#include <vector>


namespace polyscope {

// A histogram that shows up in ImGUI
class Histogram {
public:
  Histogram();                            // must call buildHistogram() with data after
  Histogram(std::vector<double>& values); // internally calls buildHistogram()

  ~Histogram();

  void buildHistogram(const std::vector<double>& values);
  void updateColormap(const std::string& newColormap);

  // Width = -1 means set automatically
  void buildUI(float width = -1.0);

  std::pair<double, double> colormapRange; // in DATA values, not [0,1]

private:
  // = Helpers

  // Manage the actual histogram
  void fillBuffers();
  size_t rawHistBinCount = 51;

  std::vector<float> rawHistCurveY;
  std::vector<std::array<float, 2>> rawHistCurveX;
  std::pair<double, double> dataRange;

  // Render to texture
  void renderToTexture();
  void prepare();

  unsigned int texDim = 600;
  std::shared_ptr<render::TextureBuffer> texture = nullptr;
  std::shared_ptr<render::FrameBuffer> framebuffer = nullptr;
  std::shared_ptr<render::ShaderProgram> program = nullptr;
  std::string colormap = "viridis";

  // A few parameters which control appearance
  float bottomBarHeight = 0.35;
  float bottomBarGap = 0.1;
};


}; // namespace polyscope
