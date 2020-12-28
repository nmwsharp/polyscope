// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/histogram.h"

#include "polyscope/affine_remapper.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <algorithm>
#include <limits>

using std::cout;
using std::endl;

namespace polyscope {

// TODO make histograms lazy. There's no need to prepare here rather than on first draw.

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

Histogram::~Histogram() {}

void Histogram::buildHistogram(std::vector<double>& values, const std::vector<double>& weights) {

  hasWeighted = weights.size() > 0;
  useWeighted = hasWeighted;

  // Build weighed and unweighted arrays of values
  size_t N = values.size();
  if (weights.size() != 0 && weights.size() != N) {
    throw std::logic_error("values and weights are not same size");
  }

  // == Build histogram
  dataRange = robustMinMax(values);
  colormapRange = dataRange;

  // Helper to build the four histogram variants
  auto buildCurve = [&](size_t binCount, bool weighted, bool smooth, std::vector<std::array<double, 2>>& curveX,
                        std::vector<double>& curveY) {
    // linspace coords
    double range = dataRange.second - dataRange.first;
    double inc = range / binCount;
    std::vector<double> sumBin(binCount, 0.0);

    // count values in buckets
    for (size_t iData = 0; iData < N; iData++) {

      double iBinf = binCount * (values[iData] - dataRange.first) / range;
      size_t iBin = std::floor(glm::clamp(iBinf, 0.0, (double)binCount - 1));

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
    double prevXEnd = dataRange.first;
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
        curveX[i][0] = (curveX[i][0] - dataRange.first) / range;
        curveX[i][1] = (curveX[i][1] - dataRange.first) / range;
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

void Histogram::updateColormap(const std::string& newColormap) {
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
  std::vector<glm::vec2> coords;

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
    coords.push_back(glm::vec2{leftX, 0.0});
    coords.push_back(glm::vec2{rightX, 0.0});
    coords.push_back(glm::vec2{leftX, leftY});

    // = Upper triangle (lower right, upper right, upper left)
    coords.push_back(glm::vec2{rightX, 0.0});
    coords.push_back(glm::vec2{rightX, rightY});
    coords.push_back(glm::vec2{leftX, leftY});
  }

  program->setAttribute("a_coord", coords);
  program->setTextureFromColormap("t_colormap", colormap, true);

  // Update current buffer settings
  currBufferWeighted = useWeighted;
  currBufferSmoothed = useSmoothed;
}

void Histogram::prepare() {

  framebuffer = render::engine->generateFrameBuffer(texDim, texDim);
  texturebuffer = render::engine->generateTextureBuffer(TextureFormat::RGBA8, texDim, texDim);
  framebuffer->addColorBuffer(texturebuffer);

  // Create the program
  program = render::engine->requestShader("HISTOGRAM", {}, render::ShaderReplacementDefaults::Process);

  prepared = true;
}


void Histogram::renderToTexture() {

  // Refill buffer if needed
  if (currBufferWeighted != useWeighted || currBufferSmoothed != useSmoothed) {
    fillBuffers();
  }

  framebuffer->clearColor = {0.0, 0.0, 0.0};
  framebuffer->clearAlpha = 0.2;
  framebuffer->setViewport(0, 0, texDim, texDim);
  framebuffer->bindForRendering();
  framebuffer->clear();

  // = Set uniforms

  // Colormap range (remapped to the 0-1 coords we use)
  program->setUniform("u_cmapRangeMin", (colormapRange.first - dataRange.first) / (dataRange.second - dataRange.first));
  program->setUniform("u_cmapRangeMax",
                      (colormapRange.second - dataRange.first) / (dataRange.second - dataRange.first));

  // Draw
  program->draw();
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
  ImGui::Image(texturebuffer->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

  // Draw a cursor popup on mouseover
  if (ImGui::IsItemHovered()) {

    // Get mouse x coodinate within image
    float mouseX = ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x - ImGui::GetScrollX();
    double mouseT = mouseX / w;
    double val = dataRange.first + mouseT * (dataRange.second - dataRange.first);

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
