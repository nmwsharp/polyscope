#include "polyscope/point_cloud_scalar_quantity.h"

#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {


PointCloudScalarQuantity::PointCloudScalarQuantity(std::string name, const std::vector<double>& values_,
                                                   PointCloud& pointCloud_, DataType dataType_)
    : PointCloudQuantityThatDrawsPoints(name, pointCloud_), dataType(dataType_)

{
  if (values_.size() != parent.points.size()) {
    polyscope::error("Point cloud scalar quantity " + name + " does not have same number of values (" +
                     std::to_string(values_.size()) + ") as point cloud size (" + std::to_string(parent.points.size()) +
                     ")");
  }

  // Set the default colormap based on what kind of data is given
  switch (dataType) {
  case DataType::STANDARD:
    iColorMap = gl::getColormapIndex_quantitative("viridis");
    break;
  case DataType::SYMMETRIC:
    iColorMap = gl::getColormapIndex_quantitative("coolwarm");
    break;
  case DataType::MAGNITUDE:
    iColorMap = gl::getColormapIndex_quantitative("blues");
    break;
  }

  // Copy the raw data
  values = values_;

  hist.updateColormap(gl::quantitativeColormaps[iColorMap]);
  hist.buildHistogram(values);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(values, 1e-5);
  resetVizRange();
}

void PointCloudScalarQuantity::draw() {
  if (!enabled) return;

  // Make the program if we don't have one already
  if (pointProgram == nullptr) {
    createPointProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*pointProgram);
  parent.setPointCloudUniforms(*pointProgram);
  pointProgram->setUniform("u_rangeLow", vizRangeLow);
  pointProgram->setUniform("u_rangeHigh", vizRangeHigh);

  pointProgram->draw();
}

void PointCloudScalarQuantity::resetVizRange() {
  switch (dataType) {
  case DataType::STANDARD:
    vizRangeLow = dataRangeLow;
    vizRangeHigh = dataRangeHigh;
    break;
  case DataType::SYMMETRIC: {
    float absRange = std::max(std::abs(dataRangeLow), std::abs(dataRangeHigh));
    vizRangeLow = -absRange;
    vizRangeHigh = absRange;
  } break;
  case DataType::MAGNITUDE:
    vizRangeLow = 0.0;
    vizRangeHigh = dataRangeHigh;
    break;
  }
}

void PointCloudScalarQuantity::buildCustomUI() {
  ImGui::SameLine();

  { // Set colormap
    ImGui::SameLine();
    ImGui::PushItemWidth(100);
    int iColormapBefore = iColorMap;
    ImGui::Combo("##colormap", &iColorMap, gl::quantitativeColormapNames, IM_ARRAYSIZE(gl::quantitativeColormapNames));
    ImGui::PopItemWidth();
    if (iColorMap != iColormapBefore) {
      pointProgram.reset();
      hist.updateColormap(gl::quantitativeColormaps[iColorMap]);
    }
  }

  // Reset button
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    resetVizRange();
  }

  // Draw the histogram of values
  hist.colormapRangeMin = vizRangeLow;
  hist.colormapRangeMax = vizRangeHigh;
  hist.buildUI();

  // Data range
  // Note: %g specifiers are generally nicer than %e, but here we don't acutally have a choice. ImGui (for somewhat
  // valid reasons) links the resolution of the slider to the decimal width of the formatted number. When %g formats a
  // number with few decimal places, sliders can break. There is no way to set a minimum number of decimal places with
  // %g, unfortunately.
  {
    switch (dataType) {
    case DataType::STANDARD:
      ImGui::DragFloatRange2("", &vizRangeLow, &vizRangeHigh, (dataRangeHigh - dataRangeLow) / 100., dataRangeLow,
                             dataRangeHigh, "Min: %.3e", "Max: %.3e");
      break;
    case DataType::SYMMETRIC: {
      float absRange = std::max(std::abs(dataRangeLow), std::abs(dataRangeHigh));
      ImGui::DragFloatRange2("##range_symmetric", &vizRangeLow, &vizRangeHigh, absRange / 100., -absRange, absRange,
                             "Min: %.3e", "Max: %.3e");
    } break;
    case DataType::MAGNITUDE: {
      ImGui::DragFloatRange2("##range_mag", &vizRangeLow, &vizRangeHigh, vizRangeHigh / 100., 0.0, dataRangeHigh,
                             "Min: %.3e", "Max: %.3e");
    } break;
    }
  }
}


void PointCloudScalarQuantity::createPointProgram() {
  // Create the program to draw this quantity

  pointProgram.reset(new gl::GLProgram(&SPHERE_VALUE_VERT_SHADER, &SPHERE_VALUE_BILLBOARD_GEOM_SHADER,
                                       &SPHERE_VALUE_BILLBOARD_FRAG_SHADER, gl::DrawMode::Points));

  // Fill buffers
  pointProgram->setAttribute("a_position", parent.points);
  pointProgram->setAttribute("a_value", values);
  pointProgram->setTextureFromColormap("t_colormap", *gl::quantitativeColormaps[iColorMap]);

  setMaterialForProgram(*pointProgram, "wax");
}

void PointCloudScalarQuantity::buildPickUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[ind]);
  ImGui::NextColumn();
}

std::string PointCloudScalarQuantity::niceName() { return name + " (scalar)"; }
void PointCloudScalarQuantity::invalidate() { pointProgram.reset(); }

} // namespace polyscope
