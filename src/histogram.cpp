// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/histogram.h"

#include "polyscope/affine_remapper.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace polyscope {

Histogram::Histogram() {}

Histogram::Histogram(std::vector<double>& values) { buildHistogram(values); }

Histogram::~Histogram() {}

void Histogram::buildHistogram(const std::vector<double>& values) {

  // Build arrays of values
  size_t N = values.size();

  // == Build histogram
  dataRange = robustMinMax(values);
  colormapRange = dataRange;

  // Helper to build the four histogram variants
  auto buildCurve = [&](size_t binCount, std::vector<std::array<float, 2>>& curveX, std::vector<float>& curveY) {
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
        sumBin[iBin] += 1.0;
      }
    }


    // build histogram coords
    curveX = std::vector<std::array<float, 2>>(binCount);
    curveY = std::vector<float>(binCount);
    double prevSumU = 0.0;
    double prevSumW = 0.0;
    double prevXEnd = dataRange.first;
    for (size_t iBin = 0; iBin < binCount; iBin++) {
      // y value
      curveY[iBin] = sumBin[iBin];

      // x value
      double xEnd = prevXEnd + inc;
      curveX[iBin] = {{static_cast<float>(prevXEnd), static_cast<float>(xEnd)}};
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
  };

  buildCurve(rawHistBinCount, rawHistCurveX, rawHistCurveY);
}


void Histogram::updateColormap(const std::string& newColormap) {
  colormap = newColormap;
  if (program) {
    program.reset();
  }
}

void Histogram::fillBuffers() {

  if (rawHistCurveY.size() == 0) {
    exception("histogram fillBuffers() called before buildHistogram");
  }

  // Push to buffer
  std::vector<glm::vec2> coords;

  float histHeightStart = bottomBarHeight + bottomBarGap;

  for (size_t i = 0; i < rawHistCurveX.size(); i++) {

    float leftX = rawHistCurveX[i][0];
    float rightX = rawHistCurveX[i][1];

    float leftYTop = histHeightStart + (1. - histHeightStart) * rawHistCurveY[i];
    float rightYTop = histHeightStart + (1. - histHeightStart) * rawHistCurveY[i];

    float leftYBot = histHeightStart;
    float rightYBot = histHeightStart;

    // = Lower triangle (lower left, lower right, upper left)
    coords.push_back(glm::vec2{leftX, leftYBot});
    coords.push_back(glm::vec2{rightX, rightYBot});
    coords.push_back(glm::vec2{leftX, leftYTop});

    // = Upper triangle (lower right, upper right, upper left)
    coords.push_back(glm::vec2{rightX, rightYBot});
    coords.push_back(glm::vec2{rightX, rightYTop});
    coords.push_back(glm::vec2{leftX, leftYTop});
  }

  // the long skinny bar along the bottom, which always shows regardless of histogram values
  coords.push_back(glm::vec2{0., 0.});
  coords.push_back(glm::vec2{1., 0.});
  coords.push_back(glm::vec2{0., bottomBarHeight});
  coords.push_back(glm::vec2{1., 0.});
  coords.push_back(glm::vec2{1., bottomBarHeight});
  coords.push_back(glm::vec2{0., bottomBarHeight});


  program->setAttribute("a_coord", coords);
}

void Histogram::prepare() {

  framebuffer = render::engine->generateFrameBuffer(texDim, texDim);
  texture = render::engine->generateTextureBuffer(TextureFormat::RGBA8, texDim, texDim);
  framebuffer->addColorBuffer(texture);

  // Create the program
  program = render::engine->requestShader("HISTOGRAM", {}, render::ShaderReplacementDefaults::Process);

  program->setTextureFromColormap("t_colormap", colormap, true);

  fillBuffers();
}


void Histogram::renderToTexture() {

  if (!program) {
    prepare();
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

  // NOTE: I'm surprised this works, since we're drawing in the middle of imgui's processing. Possible source of bugs?
  renderToTexture();

  // Compute size for image
  float aspect = 4.0;
  float w = width;
  if (w == -1.0) {
    w = .7 * ImGui::GetWindowWidth();
  }
  float h = w / aspect;

  // Render image
  ImGui::Image(texture->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

  // Helpful info for drawing annotations below
  ImU32 annoColor = ImGui::ColorConvertFloat4ToU32(ImVec4(254 / 255., 221 / 255., 66 / 255., 1.0));
  ImU32 annoColorDark = ImGui::ColorConvertFloat4ToU32(ImVec4(5. / 255., 5. / 255., 5. / 255., 1.0));
  ImVec2 imageLowerLeft(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);

  // Draw a cursor popup on mouseover
  if (ImGui::IsItemHovered()) {
    // Get mouse x coodinate within image
    float mouseX = ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x - ImGui::GetScrollX();
    double mouseT = mouseX / w;
    double val = dataRange.first + mouseT * (dataRange.second - dataRange.first);

    ImGui::SetTooltip("%g", val);

    // Draw line
    ImVec2 lineStart(imageLowerLeft.x + mouseX, imageLowerLeft.y - h - 3);
    ImVec2 lineEnd(imageLowerLeft.x + mouseX, imageLowerLeft.y - 4);
    ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, annoColor);
  }


  /* This is pretty neat, but ultimately I decided I don't like the look of it. It has
   * some value as a more obvious user widget than dragging imgui's sliders, however.

  { // Draw triangles that indicate where the colormap is

    // clang-format off

    float markerTriWidth = 0.05;
    float imageLeft = imageLowerLeft.x;
    float imageRight = imageLeft + w;
    float imageTop = imageLowerLeft.y - 4;
    float imageBot = imageTop + h;

    float leftX = (colormapRange.first - dataRange.first) / (dataRange.second - dataRange.first);
    float rightX = (colormapRange.second - dataRange.first) / (dataRange.second - dataRange.first);
    float markerTriHeight = bottomBarHeight / 2;

    // left triangle
    ImGui::GetWindowDrawList()->AddTriangleFilled(
        {imageLeft + w*(leftX - markerTriWidth) , imageTop},
        {imageLeft + w*(leftX + markerTriWidth) , imageTop},
        {imageLeft + w*leftX                    , imageTop - h*markerTriHeight},
        annoColor
      );
    ImGui::GetWindowDrawList()->AddTriangle(
        {imageLeft + w*(leftX - markerTriWidth) , imageTop},
        {imageLeft + w*(leftX + markerTriWidth) , imageTop},
        {imageLeft + w*leftX                    , imageTop - h*markerTriHeight},
        annoColorDark
      );

    // right triangle
    ImGui::GetWindowDrawList()->AddTriangleFilled(
        {imageLeft + w*(rightX - markerTriWidth) , imageTop},
        {imageLeft + w*(rightX + markerTriWidth) , imageTop},
        {imageLeft + w*rightX                    , imageTop - h*markerTriHeight},
        annoColor
      );
    ImGui::GetWindowDrawList()->AddTriangle(
        {imageLeft + w*(rightX - markerTriWidth) , imageTop},
        {imageLeft + w*(rightX + markerTriWidth) , imageTop},
        {imageLeft + w*rightX                    , imageTop - h*markerTriHeight},
        annoColorDark
      );

    // clang-format on
  }

  */
}


} // namespace polyscope
