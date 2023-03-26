// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

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
  parent.fillNodeGeometryBuffers(*nodeProgram);
  parent.fillEdgeGeometryBuffers(*edgeProgram);

  { // Fill node color buffers
    nodeProgram->setAttribute("a_value", values.getRenderAttributeBuffer());
  }

  { // Fill edge color buffers
    edgeProgram->setAttribute("a_value_tail", values.getIndexedRenderAttributeBuffer(parent.edgeTailInds));
    edgeProgram->setAttribute("a_value_tip", values.getIndexedRenderAttributeBuffer(parent.edgeTipInds));
  }

  edgeProgram->setTextureFromColormap("t_colormap", cMap.get());
  nodeProgram->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*nodeProgram, parent.getMaterial());
  render::engine->setMaterial(*edgeProgram, parent.getMaterial());
}


void CurveNetworkNodeScalarQuantity::buildNodeInfoGUI(size_t nInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(nInd));
  ImGui::NextColumn();
}


// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

CurveNetworkEdgeScalarQuantity::CurveNetworkEdgeScalarQuantity(std::string name, const std::vector<double>& values_,
                                                               CurveNetwork& network_, DataType dataType_)
    : CurveNetworkScalarQuantity(name, network_, "edge", values_, dataType_),
      nodeAverageValues(uniquePrefix() + "#nodeAverageValues", nodeAverageValuesData) {}

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
    updateNodeAverageValues();
    nodeProgram->setAttribute("a_value", nodeAverageValues.getRenderAttributeBuffer());
  }

  { // Fill edge color buffers
    edgeProgram->setAttribute("a_value", values.getRenderAttributeBuffer());
  }

  edgeProgram->setTextureFromColormap("t_colormap", cMap.get());
  nodeProgram->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*nodeProgram, parent.getMaterial());
  render::engine->setMaterial(*edgeProgram, parent.getMaterial());
}

void CurveNetworkEdgeScalarQuantity::updateNodeAverageValues() {
  parent.edgeTailInds.ensureHostBufferPopulated();
  parent.edgeTipInds.ensureHostBufferPopulated();
  values.ensureHostBufferPopulated();
  nodeAverageValues.data.resize(parent.nNodes());

  for (size_t iE = 0; iE < parent.nEdges(); iE++) {
    size_t eTail = parent.edgeTailInds.data[iE];
    size_t eTip = parent.edgeTipInds.data[iE];

    nodeAverageValues.data[eTail] += values.data[iE];
    nodeAverageValues.data[eTip] += values.data[iE];
  }

  for (size_t iN = 0; iN < parent.nNodes(); iN++) {
    nodeAverageValues.data[iN] /= parent.nodeDegrees[iN];
    if (parent.nodeDegrees[iN] == 0) {
      nodeAverageValues.data[iN] = 0.;
    }
  }

  nodeAverageValues.markHostBufferUpdated();
}

void CurveNetworkEdgeScalarQuantity::buildEdgeInfoGUI(size_t eInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(eInd));
  ImGui::NextColumn();
}


} // namespace polyscope
