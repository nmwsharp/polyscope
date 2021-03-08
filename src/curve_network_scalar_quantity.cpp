// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/curve_network_scalar_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

CurveNetworkScalarQuantity::CurveNetworkScalarQuantity(std::string name, CurveNetwork& network_, std::string definedOn_,
                                                       const std::vector<double>& values_, DataType dataType_)
    : CurveNetworkQuantity(name, network_, true), ScalarQuantity(*this, values_, dataType_), definedOn(definedOn_) {}

void CurveNetworkScalarQuantity::draw() {
  if (!isEnabled()) return;

  if (edgeProgram == nullptr || nodeProgram == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*edgeProgram);
  parent.setStructureUniforms(*nodeProgram);

  parent.setCurveNetworkEdgeUniforms(*edgeProgram);
  parent.setCurveNetworkNodeUniforms(*nodeProgram);

  setScalarUniforms(*edgeProgram);
  setScalarUniforms(*nodeProgram);

  edgeProgram->draw();
  nodeProgram->draw();
}

void CurveNetworkScalarQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    buildScalarOptionsUI();

    ImGui::EndPopup();
  }

  buildScalarUI();
}

void CurveNetworkScalarQuantity::refresh() {
  nodeProgram.reset();
  edgeProgram.reset();
  Quantity::refresh();
}

std::string CurveNetworkScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

// ========================================================
// ==========             Node Scalar            ==========
// ========================================================

CurveNetworkNodeScalarQuantity::CurveNetworkNodeScalarQuantity(std::string name, const std::vector<double>& values_,
                                                               CurveNetwork& network_, DataType dataType_)
    : CurveNetworkScalarQuantity(name, network_, "node", values_, dataType_)

{}

void CurveNetworkNodeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  nodeProgram = render::engine->requestShader(
      "RAYCAST_SPHERE", addScalarRules(parent.addCurveNetworkNodeRules({"SPHERE_PROPAGATE_VALUE"})));
  edgeProgram = render::engine->requestShader(
      "RAYCAST_CYLINDER", addScalarRules(parent.addCurveNetworkEdgeRules({"CYLINDER_PROPAGATE_BLEND_VALUE"})));

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

CurveNetworkEdgeScalarQuantity::CurveNetworkEdgeScalarQuantity(std::string name, const std::vector<double>& values_,
                                                               CurveNetwork& network_, DataType dataType_)
    : CurveNetworkScalarQuantity(name, network_, "edge", values_, dataType_)

{}

void CurveNetworkEdgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  nodeProgram = render::engine->requestShader(
      "RAYCAST_SPHERE", addScalarRules(parent.addCurveNetworkNodeRules({"SPHERE_PROPAGATE_VALUE"})));
  edgeProgram = render::engine->requestShader(
      "RAYCAST_CYLINDER", addScalarRules(parent.addCurveNetworkEdgeRules({"CYLINDER_PROPAGATE_VALUE"})));

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
