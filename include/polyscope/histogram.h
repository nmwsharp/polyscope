// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/gl_utils.h"

#include <vector>


namespace polyscope {

// A histogram that shows up in ImGUI
class Histogram {
public:
  Histogram();
  Histogram(std::vector<double>& values);
  Histogram(std::vector<double>& values, const std::vector<double>& weights);

  ~Histogram();

  void buildHistogram(std::vector<double>& values, const std::vector<double>& weights = {});
  void updateColormap(gl::ColorMapID newColormap);

  // Width = -1 means set automatically
  void buildUI(float width = -1.0);

  float colormapRangeMin, colormapRangeMax; // in DATA values, not [0,1]

private:
  // = Helpers

  // Manage the actual histogram
  void fillBuffers();
  void smoothCurve(std::vector<std::array<double, 2>>& xVals, std::vector<double>& yVals);
  size_t smoothedHistBinCount = 201;
  size_t rawHistBinCount = 51;

  // There are 4 combinations of {weighted/unweighed}, {smoothed/raw} histograms that might be displaced.
  // We just generate them all initially
  std::vector<double> weightedRawHistCurveY;
  std::vector<double> unweightedRawHistCurveY;
  std::vector<double> weightedSmoothedHistCurveY;
  std::vector<double> unweightedSmoothedHistCurveY;
  std::vector<std::array<double, 2>> smoothedHistCurveX; // left and right sides of each bucket, in "data" coordinates
  std::vector<std::array<double, 2>> rawHistCurveX;
  double minVal;
  double maxVal;
  bool hasWeighted = false;
  bool useWeighted = false;
  bool useSmoothed = true;
  bool currBufferWeighted = false;
  bool currBufferSmoothed = false;


  // Render to texture
  void renderToTexture();
  void prepare();
  bool prepared = false;

  GLuint texDim = 600;
  gl::GLTexturebuffer* texturebuffer = nullptr;
  gl::GLFramebuffer* framebuffer = nullptr;
  gl::GLProgram* program = nullptr;
  gl::ColorMapID colormap = gl::ColorMapID::VIRIDIS;
};


}; // namespace polyscope
