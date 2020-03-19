// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/curve_network_scalar_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/shaders.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

CurveNetworkScalarQuantity::CurveNetworkScalarQuantity(std::string name, CurveNetwork& network_, std::string definedOn_,
                                                       DataType dataType_)
    : CurveNetworkQuantity(name, network_, true), dataType(dataType_),
      cMap(uniquePrefix() + name + "#cmap", defaultColorMap(dataType)), definedOn(definedOn_) {}

void CurveNetworkScalarQuantity::draw() {
  if (!isEnabled()) return;

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
void CurveNetworkScalarQuantity::setProgramUniforms(render::ShaderProgram& program) {
  program.setUniform("u_rangeLow", vizRange.first);
  program.setUniform("u_rangeHigh", vizRange.second);
}

CurveNetworkScalarQuantity* CurveNetworkScalarQuantity::resetMapRange() {
  switch (dataType) {
  case DataType::STANDARD:
    vizRange = dataRange;
    break;
  case DataType::SYMMETRIC: {
    double absRange = std::max(std::abs(dataRange.first), std::abs(dataRange.second));
    vizRange = std::make_pair(-absRange, absRange);
  } break;
  case DataType::MAGNITUDE:
    vizRange = std::make_pair(0., dataRange.second);
    break;
  }

  requestRedraw();
  return this;
}

void CurveNetworkScalarQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    if (ImGui::MenuItem("Reset colormap range")) resetMapRange();

    ImGui::EndPopup();
  }

  if (render::buildColormapSelector(cMap.get())) {
    nodeProgram.reset();
    edgeProgram.reset();
    setColorMap(getColorMap());
  }

  // Draw the histogram of values
  hist.colormapRange = vizRange;
  hist.buildUI();

  // Data range
  // Note: %g specifies are generally nicer than %e, but here we don't acutally have a choice. ImGui (for somewhat
  // valid reasons) links the resolution of the slider to the decimal width of the formatted number. When %g formats a
  // number with few decimal places, sliders can break. There is no way to set a minimum number of decimal places with
  // %g, unfortunately.
  {
    switch (dataType) {
    case DataType::STANDARD:
      ImGui::DragFloatRange2("", &vizRange.first, &vizRange.second, (dataRange.second - dataRange.first) / 100.,
                             dataRange.first, dataRange.second, "Min: %.3e", "Max: %.3e");
      break;
    case DataType::SYMMETRIC: {
      float absRange = std::max(std::abs(dataRange.first), std::abs(dataRange.second));
      ImGui::DragFloatRange2("##range_symmetric", &vizRange.first, &vizRange.second, absRange / 100., -absRange,
                             absRange, "Min: %.3e", "Max: %.3e");
    } break;
    case DataType::MAGNITUDE: {
      ImGui::DragFloatRange2("##range_mag", &vizRange.first, &vizRange.second, vizRange.second / 100., 0.0,
                             dataRange.second, "Min: %.3e", "Max: %.3e");
    } break;
    }
  }
}

void CurveNetworkScalarQuantity::geometryChanged() {
  nodeProgram.reset();
  edgeProgram.reset();
}

CurveNetworkScalarQuantity* CurveNetworkScalarQuantity::setColorMap(std::string name) {
  cMap = name;
  hist.updateColormap(cMap.get());
  requestRedraw();
  return this;
}
std::string CurveNetworkScalarQuantity::getColorMap() { return cMap.get(); }

CurveNetworkScalarQuantity* CurveNetworkScalarQuantity::setMapRange(std::pair<double, double> val) {
  vizRange = val;
  requestRedraw();
  return this;
}
std::pair<double, double> CurveNetworkScalarQuantity::getMapRange() { return vizRange; }

std::string CurveNetworkScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

// ========================================================
// ==========             Node Scalar            ==========
// ========================================================

CurveNetworkNodeScalarQuantity::CurveNetworkNodeScalarQuantity(std::string name, std::vector<double> values_,
                                                               CurveNetwork& network_, DataType dataType_)
    : CurveNetworkScalarQuantity(name, network_, "node", dataType_), values(std::move(values_))

{
  hist.updateColormap(cMap.get());
  hist.buildHistogram(values);

  dataRange = robustMinMax(values, 1e-5);
  resetMapRange();
}

void CurveNetworkNodeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  nodeProgram = render::engine->generateShaderProgram({render::SPHERE_VALUE_VERT_SHADER,
                                                       render::SPHERE_VALUE_BILLBOARD_GEOM_SHADER,
                                                       render::SPHERE_VALUE_BILLBOARD_FRAG_SHADER},
                                                      DrawMode::Points);
  edgeProgram = render::engine->generateShaderProgram({render::CYLINDER_BLEND_VALUE_VERT_SHADER,
                                                       render::CYLINDER_BLEND_VALUE_GEOM_SHADER,
                                                       render::CYLINDER_BLEND_VALUE_FRAG_SHADER},
                                                      DrawMode::Points);

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

  edgeProgram->setTextureFromColormap("t_colormap", cMap.get());
  nodeProgram->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*nodeProgram, parent.getMaterial());
  render::engine->setMaterial(*edgeProgram, parent.getMaterial());
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
  hist.updateColormap(cMap.get());
  hist.buildHistogram(values);

  dataRange = robustMinMax(values, 1e-5);
  resetMapRange();
}

void CurveNetworkEdgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  nodeProgram = render::engine->generateShaderProgram({render::SPHERE_VALUE_VERT_SHADER,
                                                       render::SPHERE_VALUE_BILLBOARD_GEOM_SHADER,
                                                       render::SPHERE_VALUE_BILLBOARD_FRAG_SHADER},
                                                      DrawMode::Points);
  edgeProgram = render::engine->generateShaderProgram(
      {render::CYLINDER_VALUE_VERT_SHADER, render::CYLINDER_VALUE_GEOM_SHADER, render::CYLINDER_VALUE_FRAG_SHADER},
      DrawMode::Points);

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

  edgeProgram->setTextureFromColormap("t_colormap", cMap.get());
  nodeProgram->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*nodeProgram, parent.getMaterial());
  render::engine->setMaterial(*edgeProgram, parent.getMaterial());
}


void CurveNetworkEdgeScalarQuantity::buildEdgeInfoGUI(size_t eInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values[eInd]);
  ImGui::NextColumn();
}


} // namespace polyscope
