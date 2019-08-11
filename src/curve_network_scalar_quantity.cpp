// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/curve_network_scalar_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/cylinder_shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

CurveNetworkScalarQuantity::CurveNetworkScalarQuantity(std::string name, CurveNetwork& network_, std::string definedOn_,
                                                       DataType dataType_)
    : CurveNetworkQuantity(name, network_, true), dataType(dataType_), definedOn(definedOn_) {

  // Set the default colormap based on what kind of data is given
  cMap = defaultColorMap(dataType);
}

void CurveNetworkScalarQuantity::draw() {
  if (!enabled) return;

  if (edgeProgram == nullptr || nodeProgram == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*edgeProgram);
  parent.setTransformUniforms(*nodeProgram);

  parent.setCurveNetworkEdgeUniforms(*edgeProgram);
  parent.setCurveNetworkNodeUniforms(*nodeProgram);

  setProgramUniforms(*edgeProgram);
  setProgramUniforms(*nodeProgram);

  edgeProgram->draw();
  nodeProgram->draw();
}


// Update range uniforms
void CurveNetworkScalarQuantity::setProgramUniforms(gl::GLProgram& program) {
  program.setUniform("u_rangeLow", vizRangeLow);
  program.setUniform("u_rangeHigh", vizRangeHigh);
}

void CurveNetworkScalarQuantity::resetVizRange() {
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

void CurveNetworkScalarQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    if (ImGui::MenuItem("Reset colormap range")) resetVizRange();

    ImGui::EndPopup();
  }

  if (buildColormapSelector(cMap)) {
    nodeProgram.reset();
    edgeProgram.reset();
    hist.updateColormap(cMap);
  }

  // Draw the histogram of values
  hist.colormapRangeMin = vizRangeLow;
  hist.colormapRangeMax = vizRangeHigh;
  hist.buildUI();

  // Data range
  // Note: %g specifies are generally nicer than %e, but here we don't acutally have a choice. ImGui (for somewhat
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

void CurveNetworkScalarQuantity::geometryChanged() {
  nodeProgram.reset();
  edgeProgram.reset();
}

std::string CurveNetworkScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

// ========================================================
// ==========             Node Scalar            ==========
// ========================================================

CurveNetworkNodeScalarQuantity::CurveNetworkNodeScalarQuantity(std::string name, std::vector<double> values_,
                                                               CurveNetwork& network_, DataType dataType_)
    : CurveNetworkScalarQuantity(name, network_, "node", dataType_), values(std::move(values_))

{
  hist.updateColormap(cMap);
  hist.buildHistogram(values);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(values, 1e-5);
  resetVizRange();
}

void CurveNetworkNodeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  nodeProgram.reset(new gl::GLProgram(&gl::SPHERE_VALUE_VERT_SHADER, &gl::SPHERE_VALUE_BILLBOARD_GEOM_SHADER,
                                      &gl::SPHERE_VALUE_BILLBOARD_FRAG_SHADER, gl::DrawMode::Points));
  edgeProgram.reset(new gl::GLProgram(&gl::CYLINDER_BLEND_VALUE_VERT_SHADER, &gl::CYLINDER_BLEND_VALUE_GEOM_SHADER,
                                      &gl::CYLINDER_VALUE_FRAG_SHADER, gl::DrawMode::Points));

  // Fill geometry buffers
  parent.fillEdgeGeometryBuffers(*edgeProgram);
  parent.fillNodeGeometryBuffers(*nodeProgram);

  { // Fill node color buffers
    nodeProgram->setAttribute("a_value", values);
  }

  { // Fill edge color buffers
    std::vector<double> valueTail(parent.nEdges());
    std::vector<double> valueTip(parent.nEdges());
    for (size_t iE = 0; iE < parent.nEdges(); iE++) {
      auto& edge = parent.edges[iE];
      size_t eTail = std::get<0>(edge);
      size_t eTip = std::get<1>(edge);
      valueTail[iE] = values[eTail];
      valueTip[iE] = values[eTip];
    }

    edgeProgram->setAttribute("a_value_tail", valueTail);
    edgeProgram->setAttribute("a_value_tip", valueTip);
  }

  edgeProgram->setTextureFromColormap("t_colormap", getColorMap(cMap));
  nodeProgram->setTextureFromColormap("t_colormap", getColorMap(cMap));
  setMaterialForProgram(*edgeProgram, "wax");
  setMaterialForProgram(*nodeProgram, "wax");
}


void CurveNetworkNodeScalarQuantity::buildNodeInfoGUI(size_t nInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[nInd]);
  ImGui::NextColumn();
}


// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

CurveNetworkEdgeScalarQuantity::CurveNetworkEdgeScalarQuantity(std::string name, std::vector<double> values_,
                                                               CurveNetwork& network_, DataType dataType_)
    : CurveNetworkScalarQuantity(name, network_, "edge", dataType_), values(std::move(values_))

{
  hist.updateColormap(cMap);
  hist.buildHistogram(values);

  std::tie(dataRangeLow, dataRangeHigh) = robustMinMax(values, 1e-5);
  resetVizRange();
}

void CurveNetworkEdgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  nodeProgram.reset(new gl::GLProgram(&gl::SPHERE_VALUE_VERT_SHADER, &gl::SPHERE_VALUE_BILLBOARD_GEOM_SHADER,
                                      &gl::SPHERE_VALUE_BILLBOARD_FRAG_SHADER, gl::DrawMode::Points));
  edgeProgram.reset(new gl::GLProgram(&gl::CYLINDER_VALUE_VERT_SHADER, &gl::CYLINDER_VALUE_GEOM_SHADER,
                                      &gl::CYLINDER_VALUE_FRAG_SHADER, gl::DrawMode::Points));

  // Fill geometry buffers
  parent.fillEdgeGeometryBuffers(*edgeProgram);
  parent.fillNodeGeometryBuffers(*nodeProgram);

  { // Fill node color buffers
    // Compute an average color at each node
    std::vector<double> averageValueNode(parent.nNodes(), 0.);
    for (size_t iE = 0; iE < parent.nEdges(); iE++) {
      auto& edge = parent.edges[iE];
      size_t eTail = std::get<0>(edge);
      size_t eTip = std::get<1>(edge);
      averageValueNode[eTail] += values[iE];
      averageValueNode[eTip] += values[iE];
    }

    for (size_t iN = 0; iN < parent.nNodes(); iN++) {
      averageValueNode[iN] /= parent.nodeDegrees[iN];
    }

    nodeProgram->setAttribute("a_value", averageValueNode);
  }

  { // Fill edge color buffers
    edgeProgram->setAttribute("a_value", values);
  }

  edgeProgram->setTextureFromColormap("t_colormap", getColorMap(cMap));
  nodeProgram->setTextureFromColormap("t_colormap", getColorMap(cMap));
  setMaterialForProgram(*edgeProgram, "wax");
  setMaterialForProgram(*nodeProgram, "wax");
}


void CurveNetworkEdgeScalarQuantity::buildEdgeInfoGUI(size_t eInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[eInd]);
  ImGui::NextColumn();
}


} // namespace polyscope
