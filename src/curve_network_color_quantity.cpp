// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/curve_network_color_quantity.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/shaders.h"

#include "imgui.h"

namespace polyscope {

CurveNetworkColorQuantity::CurveNetworkColorQuantity(std::string name, CurveNetwork& network_, std::string definedOn_)
    : CurveNetworkQuantity(name, network_, true), definedOn(definedOn_) {}

void CurveNetworkColorQuantity::draw() {
  if (!isEnabled()) return;

  if (edgeProgram == nullptr || nodeProgram == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*edgeProgram);
  parent.setTransformUniforms(*nodeProgram);

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
    : CurveNetworkColorQuantity(name, network_, "node"), values(std::move(values_))

{}

void CurveNetworkNodeColorQuantity::createProgram() {
  // Create the program to draw this quantity
  nodeProgram = render::engine->generateShaderProgram({render::SPHERE_COLOR_VERT_SHADER,
                                                       render::SPHERE_COLOR_BILLBOARD_GEOM_SHADER,
                                                       render::SPHERE_COLOR_BILLBOARD_FRAG_SHADER},
                                                      DrawMode::Points);
  edgeProgram = render::engine->generateShaderProgram({render::CYLINDER_BLEND_COLOR_VERT_SHADER,
                                                       render::CYLINDER_BLEND_COLOR_GEOM_SHADER,
                                                       render::CYLINDER_BLEND_COLOR_FRAG_SHADER},
                                                      DrawMode::Points);

  // Fill geometry buffers
  parent.fillEdgeGeometryBuffers(*edgeProgram);
  parent.fillNodeGeometryBuffers(*nodeProgram);

  { // Fill node color buffers
    nodeProgram->setAttribute("a_color", values);
  }

  { // Fill edge color buffers
    std::vector<glm::vec3> colorTail(parent.nEdges());
    std::vector<glm::vec3> colorTip(parent.nEdges());
    for (size_t iE = 0; iE < parent.nEdges(); iE++) {
      auto& edge = parent.edges[iE];
      size_t eTail = std::get<0>(edge);
      size_t eTip = std::get<1>(edge);
      colorTail[iE] = values[eTail];
      colorTip[iE] = values[eTip];
    }

    edgeProgram->setAttribute("a_color_tail", colorTail);
    edgeProgram->setAttribute("a_color_tip", colorTip);
  }

  render::engine->setMaterial(*nodeProgram, parent.getMaterial());
  render::engine->setMaterial(*edgeProgram, parent.getMaterial());
}


void CurveNetworkNodeColorQuantity::buildNodeInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = values[vInd];
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::string colorStr = to_string_short(tempColor);
  ImGui::TextUnformatted(colorStr.c_str());
  ImGui::NextColumn();
}

std::string CurveNetworkColorQuantity::niceName() { return name + " (" + definedOn + " color)"; }

void CurveNetworkColorQuantity::geometryChanged() {
  nodeProgram.reset();
  edgeProgram.reset();
}

// ========================================================
// ==========            Edge Color              ==========
// ========================================================

CurveNetworkEdgeColorQuantity::CurveNetworkEdgeColorQuantity(std::string name, std::vector<glm::vec3> values_,
                                                             CurveNetwork& network_)
    : CurveNetworkColorQuantity(name, network_, "edge"), values(std::move(values_))

{}

void CurveNetworkEdgeColorQuantity::createProgram() {
  nodeProgram = render::engine->generateShaderProgram({render::SPHERE_COLOR_VERT_SHADER,
                                                       render::SPHERE_COLOR_BILLBOARD_GEOM_SHADER,
                                                       render::SPHERE_COLOR_BILLBOARD_FRAG_SHADER},
                                                      DrawMode::Points);
  edgeProgram = render::engine->generateShaderProgram(
      {render::CYLINDER_COLOR_VERT_SHADER, render::CYLINDER_COLOR_GEOM_SHADER, render::CYLINDER_COLOR_FRAG_SHADER},
      DrawMode::Points);

  // Fill geometry buffers
  parent.fillEdgeGeometryBuffers(*edgeProgram);
  parent.fillNodeGeometryBuffers(*nodeProgram);

  { // Fill node color buffers

    // Compute an average color at each node
    std::vector<glm::vec3> averageColorNode(parent.nNodes(), glm::vec3{0., 0., 0.});
    for (size_t iE = 0; iE < parent.nEdges(); iE++) {
      auto& edge = parent.edges[iE];
      size_t eTail = std::get<0>(edge);
      size_t eTip = std::get<1>(edge);
      averageColorNode[eTail] += values[iE];
      averageColorNode[eTip] += values[iE];
    }

    for (size_t iN = 0; iN < parent.nNodes(); iN++) {
      averageColorNode[iN] /= parent.nodeDegrees[iN];
    }

    nodeProgram->setAttribute("a_color", averageColorNode);
  }

  { // Fill edge color buffers
    edgeProgram->setAttribute("a_color", values);
  }

  render::engine->setMaterial(*nodeProgram, parent.getMaterial());
  render::engine->setMaterial(*edgeProgram, parent.getMaterial());
}


void CurveNetworkEdgeColorQuantity::buildEdgeInfoGUI(size_t eInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = values[eInd];
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << values[eInd];
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}

} // namespace polyscope
