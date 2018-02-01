#include "polyscope/histogram.h"

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/shaders/histogram_shaders.h"
#include "polyscope/polyscope.h"

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

Histogram::~Histogram() {
  if (prepared) {
    unprepare();
  }
}

void Histogram::buildHistogram(std::vector<double>& values, const std::vector<double>& weights) {

  hasWeighted = weights.size() > 0;
  useWeighted = hasWeighted;

  // Build weighed and unweighted arrays of values
  size_t N = values.size();
  if (weights.size() != 0 && weights.size() != N) {
    throw std::logic_error("values and weights are not same size");
  }

  // == Build histogram
  std::pair<double, double> minmax = robustMinMax(values);
  minVal = minmax.first;
  maxVal = minmax.second;
  colormapRangeMin = minVal;
  colormapRangeMax = maxVal;

  // Helper to build the four histogram variants
  auto buildCurve = [&](size_t binCount, bool weighted, bool smooth, std::vector<std::array<double, 2>>& curveX,
                        std::vector<double>& curveY) {

    // linspace coords
    double range = maxVal - minVal;
    double inc = range / binCount;
    std::vector<double> sumBin(binCount, 0.0);

    // count values in buckets
    for (size_t iData = 0; iData < N; iData++) {

      double iBinf = binCount * (values[iData] - minVal) / range;
      size_t iBin = std::floor(clamp(iBinf, 0.0, (double)binCount - 1));

      // NaN values and finite values near the bottom of float range lead to craziness, so only increment bins if we got
      // something reasonable
      if (iBin < binCount) {
        if (weighted) {
          sumBin[iBin] += weights[iData];
        } else {
          sumBin[iBin] += 1.0;
        }
      }
    }


    // build histogram coords
    curveX = std::vector<std::array<double, 2>>(binCount);
    curveY = std::vector<double>(binCount);
    double prevSumU = 0.0;
    double prevSumW = 0.0;
    double prevXEnd = minVal;
    for (size_t iBin = 0; iBin < binCount; iBin++) {
      // y value
      curveY[iBin] = sumBin[iBin];

      // x value
      double xEnd = prevXEnd + inc;
      curveX[iBin] = {{prevXEnd, xEnd}};
      prevXEnd = xEnd;
    }

    { // Rescale curves to [0,1] in both dimensions
      double maxHeight = *std::max_element(curveY.begin(), curveY.end());
      for (size_t i = 0; i < binCount; i++) {
        curveX[i][0] = (curveX[i][0] - minVal) / range;
        curveX[i][1] = (curveX[i][1] - minVal) / range;
        curveY[i] /= maxHeight;
      }
    }

    if (smooth) {
      smoothCurve(curveX, curveY);

      { // Rescale again after smoothing
        double maxHeight = *std::max_element(curveY.begin(), curveY.end());
        for (size_t i = 0; i < binCount; i++) {
          curveY[i] /= maxHeight;
        }
      }
    }

  };

  // Build the four variants of the curve
  buildCurve(rawHistBinCount, false, false, rawHistCurveX, unweightedRawHistCurveY);
  buildCurve(smoothedHistBinCount, false, true, smoothedHistCurveX, unweightedSmoothedHistCurveY);
  if (hasWeighted) {
    buildCurve(rawHistBinCount, true, false, rawHistCurveX, weightedRawHistCurveY);
    buildCurve(smoothedHistBinCount, true, true, smoothedHistCurveX, weightedSmoothedHistCurveY);
  }


  fillBuffers();
}

void Histogram::smoothCurve(std::vector<std::array<double, 2>>& xVals, std::vector<double>& yVals) {

  auto smoothFunc = [&](double x1, double x2) {
    // Tent
    // double radius = 0.1;
    // double val = (radius - std::abs(x1 - x2)) / radius;
    // return std::max(val, 0.0);

    // Gaussian
    double widthFactor = 1000;
    double dist = (x1 - x2);
    return std::exp(-dist * dist * widthFactor);

    // None
    // if(x1 == x2) return 1.0;
    // return 0.0;
  };

  std::vector<double> smoothedVals(yVals.size());
  for (size_t i = 0; i < yVals.size(); i++) {
    double bucketCi = 0.5 * (xVals[i][0] + xVals[i][1]);
    double sum = 0.0;
    for (size_t j = 0; j < yVals.size(); j++) {
      double bucketCj = 0.5 * (xVals[j][0] + xVals[j][1]);
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

  // Fill from proper curve depending on current settings
  // (does unecessary copy as written)
  std::vector<double> histCurveY;
  std::vector<std::array<double, 2>> histCurveX;
  bool smoothBins = false; // draw trapezoids rather than rectangles
  if (useSmoothed) {
    if (useWeighted) {
      histCurveY = weightedSmoothedHistCurveY;
    } else {
      histCurveY = unweightedSmoothedHistCurveY;
    }
    histCurveX = smoothedHistCurveX;
    smoothBins = true;
  } else {
    if (useWeighted) {
      histCurveY = weightedRawHistCurveY;
    } else {
      histCurveY = unweightedRawHistCurveY;
    }
    histCurveX = rawHistCurveX;
  }

  // Push to buffer
  std::vector<Vector2> coords;

  if (histCurveY.size() == 0) {
    program->setAttribute("a_coord", coords);
    return;
  }

  // Extra first triangle
  for (size_t i = 0; i < histCurveX.size(); i++) {

    double leftX = histCurveX[i][0];
    double rightX = histCurveX[i][1];

    double leftY = histCurveY[i];
    double rightY = histCurveY[i];
    if (smoothBins) {
      if (i > 0) {
        leftY = 0.5 * (histCurveY[i - 1] + histCurveY[i]);
      }
      if (i < histCurveX.size() - 1) {
        rightY = 0.5 * (histCurveY[i] + histCurveY[i + 1]);
      }
    }

    // = Lower triangle (lower left, lower right, upper left)
    coords.push_back(Vector2{leftX, 0.0});
    coords.push_back(Vector2{rightX, 0.0});
    coords.push_back(Vector2{leftX, leftY});

    // = Upper triangle (lower right, upper right, upper left)
    coords.push_back(Vector2{rightX, 0.0});
    coords.push_back(Vector2{rightX, rightY});
    coords.push_back(Vector2{leftX, leftY});
  }

  program->setAttribute("a_coord", coords);
  program->setTextureFromColormap("t_colormap", *colormap, true);


  // Update current buffer settings
  currBufferWeighted = useWeighted;
  currBufferSmoothed = useSmoothed;
}

void Histogram::prepare() {

  if (prepared) {
    unprepare();
  }

  // Generate a framebuffer to hold our texture
  glGenFramebuffers(1, &framebufferInd);
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferInd);

  // Create the texture
  glGenTextures(1, &textureInd);

  // Bind to the new texture so we can set things about it
  glBindTexture(GL_TEXTURE_2D, textureInd);

  // Configure setttings
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texDim, texDim, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

  prepared = true;
}

void Histogram::unprepare() {
  safeDelete(program);
  glDeleteTextures(1, &textureInd);
  glDeleteFramebuffers(1, &framebufferInd);
  prepared = false;
}

void Histogram::renderToTexture() {

  // Refill buffer if needed
  if (currBufferWeighted != useWeighted || currBufferSmoothed != useSmoothed) {
    fillBuffers();
  }

  // Bind to the texture buffer
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferInd);

  // Bind to the new texture so we can do things
  glBindTexture(GL_TEXTURE_2D, textureInd);


  GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, drawBuffers);

  // Make sure we render to the whole buffer
  glViewport(0, 0, texDim, texDim);
  glClearColor(0.0, 0.0, 0.0, 0.2);
  glClear(GL_COLOR_BUFFER_BIT);

  // Set uniforms

  // Colormap range (remapped to the 0-1 coords we use)
  program->setUniform("u_cmapRangeMin", (colormapRangeMin - minVal) / (maxVal - minVal));
  program->setUniform("u_cmapRangeMax", (colormapRangeMax - minVal) / (maxVal - minVal));


  // Draw
  program->draw();

  bindDefaultBuffer();
}


void Histogram::buildUI(float width) {
  renderToTexture();

  // Compute size for image
  float aspect = 3.0;
  float w = width;
  if (w == -1.0) {
    w = .8 * ImGui::GetWindowWidth();
  }
  float h = w / aspect;

  // Render image
  ImGui::Image(reinterpret_cast<void*>((size_t)textureInd) /* yes, really. */, ImVec2(w, h), ImVec2(0, 1),
               ImVec2(1, 0));

  // Draw a cursor popup on mouseover
  if (ImGui::IsItemHovered()) {

    // Get mouse x coodinate within image
    float mouseX = ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x - ImGui::GetScrollX();
    double mouseT = mouseX / w;
    double val = minVal + mouseT * (maxVal - minVal);

    ImGui::SetTooltip("%g", val);

    // Draw line
    ImVec2 imageUpperLeft(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);
    ImVec2 lineStart(imageUpperLeft.x + mouseX, imageUpperLeft.y - h - 3);
    ImVec2 lineEnd(imageUpperLeft.x + mouseX, imageUpperLeft.y - 4);
    ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd,
                                        ImGui::ColorConvertFloat4ToU32(ImVec4(254 / 255., 221 / 255., 66 / 255., 1.0)));
  }

  // Right-click combobox to select weighted/unweighted
  if (ImGui::BeginPopupContextItem("select type")) {
    if (hasWeighted) {
      ImGui::Checkbox("Weighted", &useWeighted);
    }
    ImGui::Checkbox("Smoothed", &useSmoothed);
    ImGui::EndPopup();
  }
}


} // namespace polyscope
