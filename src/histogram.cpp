#include "polyscope/histogram.h"

#include "polyscope/gl/shaders/histogram_shaders.h"
#include "polyscope/affine_remapper.h"

#include "imgui.h"

#include <algorithm>
#include <limits>

using std::cout;
using std::endl;

namespace polyscope {

Histogram::Histogram() {
  prepare();
  fillBuffers();
}

Histogram::Histogram(std::vector<double>& values) {
  prepare();
  buildHistogram(values);
}

Histogram::Histogram(std::vector<double>& values, const std::vector<double>& weights) {
  prepare();
  buildHistogram(values, weights);
}

Histogram::~Histogram() { safeDelete(program); }

void Histogram::buildHistogram(std::vector<double>& values, const std::vector<double>& weights) {

  hasWeighted = weights.size() > 0;
  useWeighted = hasWeighted;

  // Build weighed and unweighted arrays of values
  size_t N = values.size();
  if (weights.size() != 0 && weights.size() != N) {
    throw std::logic_error("values and weights are not same size");
  }

  double totalWeight = 0;
  if (hasWeighted) {
    for (size_t i = 0; i < N; i++) {
      totalWeight += weights[i];
    }
  }

  // == Build histogram
  std::sort(values.begin(), values.end());

  std::pair<double,double> minmax = robustMinMax(values);
  minVal = minmax.first;
  maxVal = minmax.second;

  // linspace coords
  double range = maxVal - minVal;
  double inc = range / histBinCount;
  std::vector<double> sumLessThan(histBinCount, 0.0);

  // cumulative sum of values
  size_t jBin = 0;
  size_t jData = 0;
  double binUpperLim = minVal + inc;
  while (jBin < histBinCount && jData < values.size()) {
    if (values[jData] < binUpperLim) {
      sumLessThan[jBin] += 1;
      jData++;
    } else {
      jBin++;
      binUpperLim += inc;
      if (jBin < histBinCount) {
        sumLessThan[jBin] = sumLessThan[jBin - 1];
      }
    }
  }

  // build histogram coords
  histCurveX.resize(histBinCount);
  unweightedHistCurveY.resize(histBinCount);
  double prevSum = 0.0;
  double prevXEnd = minVal;
  for (size_t iBin = 0; iBin < histBinCount; iBin++) {
    // y value
    unweightedHistCurveY[iBin] = sumLessThan[iBin] - prevSum;
    prevSum = sumLessThan[iBin];

    // x value
    double xEnd = prevXEnd + inc;
    histCurveX[iBin] = {{prevXEnd, xEnd}};
    prevXEnd = xEnd;
  }

  { // Rescale curves to [0,1] in both dimensions
    double maxHeight = *std::max_element(unweightedHistCurveY.begin(), unweightedHistCurveY.end());
    for (size_t i = 0; i < histBinCount; i++) {
      histCurveX[i][0] = (histCurveX[i][0] - minVal) / range;
      histCurveX[i][1] = (histCurveX[i][1] - minVal) / range;
      unweightedHistCurveY[i] /= maxHeight;
    }
  }

  // Smooth
  smoothCurve(unweightedHistCurveY);
  // smoothCurve(weightedHistCurveY);

  { // Rescale Y again after smoothing
    double maxHeight = *std::max_element(unweightedHistCurveY.begin(), unweightedHistCurveY.end());
    for (size_t i = 0; i < histBinCount; i++) {
      unweightedHistCurveY[i] /= maxHeight;
    }
  }


  fillBuffers();
}

void Histogram::smoothCurve(std::vector<double>& yVals) {

  auto smoothFunc = [&](double x1, double x2) {
    // Tent
    //double radius = 0.1;
    //double val = (radius - std::abs(x1 - x2)) / radius;
    //return std::max(val, 0.0);
 
    // Gaussian
    double widthFactor = 1000;
    double dist = (x1 - x2);
    return std::exp(-dist*dist*widthFactor);

    // None
    //if(x1 == x2) return 1.0;
    //return 0.0;
  };

  std::vector<double> smoothedVals(yVals.size());
  for (size_t i = 0; i < yVals.size(); i++) {
    double bucketCi = 0.5 * (histCurveX[i][0] + histCurveX[i][1]);
    double sum = 0.0;
    for (size_t j = 0; j < yVals.size(); j++) {
      double bucketCj = 0.5 * (histCurveX[j][0] + histCurveX[j][1]);
      double weight = smoothFunc(bucketCi, bucketCj);
      sum += weight * yVals[j];
    }
    smoothedVals[i] = sum;
  }

  yVals = smoothedVals;
}

void Histogram::updateColormap(const gl::Colormap* newColormap) {
  colormap = newColormap;
  fillBuffers(); 
}

void Histogram::fillBuffers() {

  std::vector<double>& histCurveY = useWeighted ? weightedHistCurveY : unweightedHistCurveY;

  // Push to buffer
  std::vector<Vector2> coords;

  if (histCurveY.size() == 0) {
    program->setAttribute("a_coord", coords);
    return;
  }

  // Extra first triangle
  coords.push_back(Vector2{0.0, 0.0});
  coords.push_back(Vector2{0.5 * (histCurveX[0][0] + histCurveX[0][1]), 0.0});
  coords.push_back(Vector2{0.5 * (histCurveX[0][0] + histCurveX[0][1]), histCurveY[0]});


  for (size_t i = 0; i < histCurveX.size() - 1; i++) {

    double leftX = 0.5 * (histCurveX[i][0] + histCurveX[i][1]);
    double rightX = 0.5 * (histCurveX[i + 1][0] + histCurveX[i + 1][1]);

    // = Lower triangle (lower left, lower right, upper left)
    coords.push_back(Vector2{leftX, 0.0});
    coords.push_back(Vector2{rightX, 0.0});
    coords.push_back(Vector2{leftX, histCurveY[i]});

    // = Upper triangle (lower right, upper right, upper left)
    coords.push_back(Vector2{rightX, 0.0});
    coords.push_back(Vector2{rightX, histCurveY[i + 1]});
    coords.push_back(Vector2{leftX, histCurveY[i]});
  }

  // Extra last triangle
  coords.push_back(Vector2{0.5 * (histCurveX.back()[0] + histCurveX.back()[1]), 0.0});
  coords.push_back(Vector2{1.0, 0.0});
  coords.push_back(Vector2{0.5 * (histCurveX.back()[0] + histCurveX.back()[1]), histCurveY.back()});

  program->setAttribute("a_coord", coords);
  program->setTextureFromColormap("t_colormap", *colormap, true);
}

void Histogram::prepare() {
  // Generate a framebuffer to hold our texture
  glGenFramebuffers(1, &framebufferInd);
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferInd);

  // Create the texture
  glGenTextures(1, &textureInd);

  // Bind to the new texture so we can set things about it
  glBindTexture(GL_TEXTURE_2D, textureInd);

  // Configure setttings
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texDim, texDim, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // TODO FIXME not good!
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Attach the texture to the framebuffer
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureInd, 0);
  GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, drawBuffers);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    throw std::logic_error("Histogram framebuffer problem");
  }

  // Create the program
  program = new gl::GLProgram(&HISTOGRAM_VERT_SHADER, &HISTORGRAM_FRAG_SHADER, gl::DrawMode::Triangles);
}


void Histogram::renderToTexture() {

  // Bind to the texture buffer
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferInd);
  // cout << "binding to framebuffer ind " << framebufferInd << endl;

  // Bind to the new texture so we can do things
  glBindTexture(GL_TEXTURE_2D, textureInd);

  GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, drawBuffers);

  // Make sure we render to the whole buffer
  glViewport(0, 0, texDim, texDim);
  glClearColor(0.0, 0.2, 0.2, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  // Set uniforms


  // Draw
  program->draw();
  // cout << "drawed" << endl;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Histogram::buildUI() {
  renderToTexture();
  ImGui::Image(reinterpret_cast<void*>((size_t)textureInd) /* yes, really. */, ImVec2(300, 100), ImVec2(0, 1),
               ImVec2(1, 0));
}


} // namespace polyscope
