// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/curve_network_color_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

CurveNetworkColorQuantity::CurveNetworkColorQuantity(std::string name, CurveNetwork& network_, std::string definedOn_,
                                                     const std::vector<glm::vec3>& colorValues_)
    : CurveNetworkQuantity(name, network_, true), ColorQuantity(*this, colorValues_), definedOn(definedOn_) {}

void CurveNetworkColorQuantity::draw() {
  if (!isEnabled()) return;

  if (edgeProgram == nullptr || nodeProgram == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*edgeProgram);
  parent.setStructureUniforms(*nodeProgram);

  parent.setCurveNetworkEdgeUniforms(*edgeProgram);
  parent.setCurveNetworkNodeUniforms(*nodeProgram);

  edgeProgram->draw();
  nodeProgram->draw();
}

// ========================================================
// ==========           Node Color            ==========
// ========================================================

CurveNetworkNodeColorQuantity::CurveNetworkNodeColorQuantity(std::string name, std::vector<glm::vec3> values_,
                                                             CurveNetwork& network_)
    : CurveNetworkColorQuantity(name, network_, "node", values_) {}

void CurveNetworkNodeColorQuantity::createProgram() {

  // Create the program to draw this quantity
  nodeProgram = render::engine->requestShader(
      "RAYCAST_SPHERE", parent.addCurveNetworkNodeRules({"SPHERE_PROPAGATE_COLOR", "SHADE_COLOR"}));
  edgeProgram = render::engine->requestShader(
      "RAYCAST_CYLINDER", parent.addCurveNetworkEdgeRules({"CYLINDER_PROPAGATE_BLEND_COLOR", "SHADE_COLOR"}));

  // Fill geometry buffers
  parent.fillEdgeGeometryBuffers(*edgeProgram);
  parent.fillNodeGeometryBuffers(*nodeProgram);

  { // Fill node color buffers
    nodeProgram->setAttribute("a_color", colors.getRenderAttributeBuffer());
  }

  { // Fill edge color buffers
    edgeProgram->setAttribute("a_color_tail", colors.getIndexedRenderAttributeBuffer(parent.edgeTailInds));
    edgeProgram->setAttribute("a_color_tip", colors.getIndexedRenderAttributeBuffer(parent.edgeTipInds));
  }

  render::engine->setMaterial(*nodeProgram, parent.getMaterial());
  render::engine->setMaterial(*edgeProgram, parent.getMaterial());
}


void CurveNetworkNodeColorQuantity::buildNodeInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = colors.getValue(vInd);
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::string colorStr = to_string_short(tempColor);
  ImGui::TextUnformatted(colorStr.c_str());
  ImGui::NextColumn();
}

std::string CurveNetworkColorQuantity::niceName() { return name + " (" + definedOn + " color)"; }

void CurveNetworkColorQuantity::refresh() {
  nodeProgram.reset();
  edgeProgram.reset();
  Quantity::refresh();
}

// ========================================================
// ==========            Edge Color              ==========
// ========================================================

CurveNetworkEdgeColorQuantity::CurveNetworkEdgeColorQuantity(std::string name, std::vector<glm::vec3> values_,
                                                             CurveNetwork& network_)
    : CurveNetworkColorQuantity(name, network_, "edge", values_),
      nodeAverageColors(uniquePrefix() + "#nodeAverageColors", nodeAverageColorsData) {}

void CurveNetworkEdgeColorQuantity::createProgram() {

  nodeProgram = render::engine->requestShader(
      "RAYCAST_SPHERE", parent.addCurveNetworkNodeRules({"SPHERE_PROPAGATE_COLOR", "SHADE_COLOR"}));
  edgeProgram = render::engine->requestShader(
      "RAYCAST_CYLINDER", parent.addCurveNetworkEdgeRules({"CYLINDER_PROPAGATE_COLOR", "SHADE_COLOR"}));

  // Fill geometry buffers
  parent.fillEdgeGeometryBuffers(*edgeProgram);
  parent.fillNodeGeometryBuffers(*nodeProgram);

  { // Fill node color buffers
    // Compute an average color at each node
    updateNodeAverageColors();
    nodeProgram->setAttribute("a_color", nodeAverageColors.getRenderAttributeBuffer());
  }

  { // Fill edge color buffers
    edgeProgram->setAttribute("a_color", colors.getRenderAttributeBuffer());
  }

  render::engine->setMaterial(*nodeProgram, parent.getMaterial());
  render::engine->setMaterial(*edgeProgram, parent.getMaterial());
}

void CurveNetworkEdgeColorQuantity::updateNodeAverageColors() {
  parent.edgeTailInds.ensureHostBufferPopulated();
  parent.edgeTipInds.ensureHostBufferPopulated();
  colors.ensureHostBufferPopulated();
  nodeAverageColors.data.resize(parent.nNodes());

  for (size_t iE = 0; iE < parent.nEdges(); iE++) {
    size_t eTail = parent.edgeTailInds.data[iE];
    size_t eTip = parent.edgeTipInds.data[iE];

    nodeAverageColors.data[eTail] += colors.data[iE];
    nodeAverageColors.data[eTip] += colors.data[iE];
  }

  for (size_t iN = 0; iN < parent.nNodes(); iN++) {
    nodeAverageColors.data[iN] /= parent.nodeDegrees[iN];
    if (parent.nodeDegrees[iN] == 0) {
      nodeAverageColors.data[iN] = glm::vec3{0., 0., 0.};
    }
  }

  nodeAverageColors.markHostBufferUpdated();
}


void CurveNetworkEdgeColorQuantity::buildEdgeInfoGUI(size_t eInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = colors.getValue(eInd);
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << tempColor;
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}

} // namespace polyscope
