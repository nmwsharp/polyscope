// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/color_bar.h"

#include "polyscope/affine_remapper.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace polyscope {

ColorBar::ColorBar(Quantity& parent_)
    : parent(parent_), onscreenColorbarEnabled(parent.uniquePrefix() + "onscreenColorbarEnabled", false) {}

ColorBar::~ColorBar() {}

void ColorBar::buildHistogram(const std::vector<float>& values, DataType dataType_) {
  dataType = dataType_;

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


void ColorBar::updateColormap(const std::string& newColormap) {
  colormap = newColormap;
  inlineHistogramProgram.reset();
  cmapTexture.reset();
}

void ColorBar::fillHistogramBuffers() {

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


  inlineHistogramProgram->setAttribute("a_coord", coords);
}

void ColorBar::prepareInlineHistogram() {

  inlineHistogramFramebuffer = render::engine->generateFrameBuffer(texDim, texDim);
  inlineHistogramTexture = render::engine->generateTextureBuffer(TextureFormat::RGBA8, texDim, texDim);
  inlineHistogramFramebuffer->addColorBuffer(inlineHistogramTexture);

  // Create the inlineHistogramProgram
  if (dataType == DataType::CATEGORICAL) {
    // for categorical data only
    inlineHistogramProgram = render::engine->requestShader("HISTOGRAM_CATEGORICAL", {"SHADE_CATEGORICAL_COLORMAP"},
                                                           render::ShaderReplacementDefaults::Process);
  } else {
    // common case
    inlineHistogramProgram = render::engine->requestShader("HISTOGRAM", {"SHADE_COLORMAP_VALUE"},
                                                           render::ShaderReplacementDefaults::Process);
  }

  inlineHistogramProgram->setTextureFromColormap("t_colormap", colormap, true);

  fillHistogramBuffers();
}


void ColorBar::renderInlineHistogramToTexture() {

  if (!inlineHistogramProgram) {
    prepareInlineHistogram();
  }

  inlineHistogramFramebuffer->clearColor = {0.0, 0.0, 0.0};
  inlineHistogramFramebuffer->clearAlpha = 0.2;
  inlineHistogramFramebuffer->setViewport(0, 0, texDim, texDim);
  inlineHistogramFramebuffer->bindForRendering();
  inlineHistogramFramebuffer->clear();

  // = Set uniforms

  if (dataType == DataType::CATEGORICAL) {
    // Used to restore [0,1] tvals to the orininal data range for the categorical int remapping
    inlineHistogramProgram->setUniform("u_dataRangeLow", dataRange.first);
    inlineHistogramProgram->setUniform("u_dataRangeHigh", dataRange.second);
  } else {
    // Colormap range (remapped to the 0-1 coords we use)
    float rangeLow = (colormapRange.first - dataRange.first) / (dataRange.second - dataRange.first);
    float rangeHigh = (colormapRange.second - dataRange.first) / (dataRange.second - dataRange.first);
    inlineHistogramProgram->setUniform("u_rangeLow", rangeLow);
    inlineHistogramProgram->setUniform("u_rangeHigh", rangeHigh);
  }

  inlineHistogramProgram->draw();
}

void ColorBar::setOnscreenColorbarEnabled(bool newEnabled) {
  onscreenColorbarEnabled.set(newEnabled);
  if (newEnabled) {
    prepareOnscreenColorBar();
  }
}
bool ColorBar::getOnscreenColorbarEnabled() { return onscreenColorbarEnabled.get(); }

void ColorBar::prepareOnscreenColorBar() {
  if (onscreenColorBarWidget) {
    // Already created, nothing to do
    return;
  }
  onscreenColorBarWidget = std::unique_ptr<OnscreenColorBarWidget>(new OnscreenColorBarWidget(*this));
}

OnscreenColorBarWidget::OnscreenColorBarWidget(ColorBar& parent_) : parent(parent_) {}

void OnscreenColorBarWidget::draw() {
  if (!parent.parent.isEnabled()) return;
  if (!parent.getOnscreenColorbarEnabled()) return;

  if (!parent.cmapTexture) {
    parent.cmapTexture = render::engine->getColorMapTexture2d(parent.colormap);
  }

  // Draw the color bar

  // NOTE: right nwo the size of the colorbar is scaled by uiScale, but the positioning/margins are not. This is
  // consistent with the other positioning/marching logic in the main gui panels. (Maybe they should all be scaled?)
  float barRegionWidth = 30.0f * options::uiScale;
  float tickRegionWidth = 90.0f * options::uiScale;
  float marginWidth = 10.0f * options::uiScale;
  float barRegionHeight = 300.0f * options::uiScale;
  float borderWidth = 2.0f * options::uiScale;
  float tickWidth = 5.0f * options::uiScale;
  ImVec2 barTopLeft(internal::lastRightSideFreeX - barRegionWidth - tickRegionWidth - marginWidth,
                    internal::lastRightSideFreeY + marginWidth + internal::imguiStackMargin);

  ImDrawList* dl = ImGui::GetBackgroundDrawList();
  ImU32 backgroundColor = IM_COL32(255, 255, 255, 180);

  // dd a semi-transparent background
  dl->AddRectFilled(ImVec2(barTopLeft.x - marginWidth, barTopLeft.y - marginWidth),
                    ImVec2(barTopLeft.x + barRegionWidth + tickRegionWidth + marginWidth,
                           barTopLeft.y + barRegionHeight + marginWidth),
                    backgroundColor);

  // draw the actual colormap gradient rectangle
  render::engine->preserveResourceUntilImguiFrameCompletes(parent.cmapTexture);
  dl->AddImage((ImTextureID)(intptr_t)parent.cmapTexture->getNativeHandle(), ImVec2(barTopLeft.x, barTopLeft.y),
               ImVec2(barTopLeft.x + barRegionWidth, barTopLeft.y + barRegionHeight), ImVec2(0, 1), ImVec2(1, 0));

  // draw the border around the map
  dl->AddRect(barTopLeft, ImVec2(barTopLeft.x + barRegionWidth, barTopLeft.y + barRegionHeight), IM_COL32(0, 0, 0, 255),
              0.f, ImDrawFlags_None, borderWidth);

  // draw text ticks
  int nTicks = 5;
  for (int i = 0; i < nTicks; i++) {
    float t = (float)i / (float)(nTicks - 1);
    float yPos = barTopLeft.y + t * (barRegionHeight - 0.5 * borderWidth);
    double val = parent.colormapRange.second - t * (parent.colormapRange.second - parent.colormapRange.first);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.4g", val);

    // Make a little tick mark
    dl->AddLine(ImVec2(barTopLeft.x + barRegionWidth - borderWidth, yPos),
                ImVec2(barTopLeft.x + barRegionWidth + tickWidth, yPos), IM_COL32_BLACK, borderWidth);

    // Draw the actual text
    ImVec2 textSize = ImGui::CalcTextSize(buffer);
    dl->AddText(ImVec2(barTopLeft.x + barRegionWidth + 2.f * tickWidth, yPos - textSize.y / 2), IM_COL32_BLACK, buffer);
  }

  internal::lastRightSideFreeX -=
      (barRegionWidth + tickRegionWidth + marginWidth + 2.f * tickWidth) + internal::imguiStackMargin;
}

void ColorBar::exportColorbarToSVG(const std::string& filename) {
  std::ofstream svgFile(filename);
  if (!svgFile.is_open()) {
    polyscope::warning("Failed to open file for SVG export: " + filename);
    return;
  }

  const render::ValueColorMap& valueColormap = render::engine->getColorMap(colormap);

  // SVG dimensions
  float svgWidth = 100.0f;
  float svgHeight = 400.0f;
  float barWidth = 30.0f;
  float tickLength = 10.0f;
  float textOffset = 15.0f;
  int nTicks = 5;

  // SVG header
  svgFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  svgFile << "<svg width=\"" << (svgWidth + 80) << "\" height=\"" << svgHeight
          << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

  // Gradient definition
  svgFile << "  <defs>\n";
  svgFile << "    <linearGradient id=\"colorbarGradient\" x1=\"0%\" y1=\"100%\" x2=\"0%\" y2=\"0%\">\n";

  // Sample colormap at multiple points
  int nSamples = 50;
  for (int i = 0; i <= nSamples; i++) {
    float t = (float)i / (float)nSamples;
    glm::vec3 color = valueColormap.getValue(t);
    int r = static_cast<int>(color.x * 255);
    int g = static_cast<int>(color.y * 255);
    int b = static_cast<int>(color.z * 255);
    svgFile << "      <stop offset=\"" << (t * 100) << "%\" style=\"stop-color:rgb(" << r << "," << g << "," << b
            << ");stop-opacity:1\" />\n";
  }

  svgFile << "    </linearGradient>\n";
  svgFile << "  </defs>\n";

  // Draw the colorbar rectangle
  svgFile << "  <rect x=\"10\" y=\"10\" width=\"" << barWidth << "\" height=\"" << (svgHeight - 20)
          << "\" fill=\"url(#colorbarGradient)\" stroke=\"black\" stroke-width=\"2\"/>\n";

  // Ticks and labels
  for (int i = 0; i < nTicks; i++) {
    float t = (float)i / (float)(nTicks - 1);
    float yPos = 10 + t * (svgHeight - 20);
    double val = colormapRange.second - t * (colormapRange.second - colormapRange.first);

    // tick mark
    svgFile << "  <line x1=\"" << (10 + barWidth) << "\" y1=\"" << yPos << "\" x2=\"" << (10 + barWidth + tickLength)
            << "\" y2=\"" << yPos << "\" stroke=\"black\" stroke-width=\"2\"/>\n";

    // text label
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.4g", val);
    svgFile << "  <text x=\"" << (10 + barWidth + textOffset) << "\" y=\"" << (yPos + 5)
            << "\" font-family=\"Arial\" font-size=\"14\" fill=\"black\">" << buffer << "</text>\n";
  }

  svgFile << "</svg>\n";
  svgFile.close();
}

void ColorBar::buildUI(float width) {

  renderInlineHistogramToTexture();

  // Compute size for image
  float aspect = 4.0;
  float w = width;
  if (w == -1.0) {
    w = .7 * ImGui::GetWindowWidth();
  }
  float h = w / aspect;

  // Render image
  ImGui::Image((ImTextureID)(intptr_t)inlineHistogramTexture->getNativeHandle(), ImVec2(w, h), ImVec2(0, 1),
               ImVec2(1, 0));
  render::engine->preserveResourceUntilImguiFrameCompletes(inlineHistogramTexture);

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


  // NOTE: the onscreen colorbar gets drawn in the draw() command, rather than with
  // the rest of the UI stuff here, because we need it to happen even if this UI
  // panel is collapsed.
}


} // namespace polyscope
