// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/curve_network.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/cylinder_shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

using std::cout;
using std::endl;

namespace polyscope {

// Initialize statics
const std::string CurveNetwork::structureTypeName = "Curve Network";

// Constructor
CurveNetwork::CurveNetwork(std::string name, std::vector<glm::vec3> nodes_, std::vector<std::array<size_t, 2>> edges_)
    : QuantityStructure<CurveNetwork>(name), nodes(std::move(nodes_)), edges(std::move(edges_)) {

  nodeDegrees = std::vector<size_t>(nNodes(), 0);

  size_t maxInd = nodes.size();
  for (size_t iE = 0; iE < edges.size(); iE++) {
    auto edge = edges[iE];
    size_t nA = std::get<0>(edge);
    size_t nB = std::get<1>(edge);

    // Make sure there are no out of bounds indices
    if (nA >= maxInd || nB >= maxInd) {
      polyscope::error("CurveNetwork [" + name + "] edge " + std::to_string(iE) + " has bad node indices { " +
                       std::to_string(nA) + " , " + std::to_string(nB) + " } but there are " + std::to_string(maxInd) +
                       " edges.");
    }

    // Increment degree
    nodeDegrees[nA]++;
    nodeDegrees[nB]++;
  }

  baseColor = getNextUniqueColor();
}


// Helper to set uniforms
void CurveNetwork::setCurveNetworkNodeUniforms(gl::GLProgram& p) {
  p.setUniform("u_pointRadius", radius * state::lengthScale);

  glm::vec3 lookDir, upDir, rightDir;
  view::getCameraFrame(lookDir, upDir, rightDir);
  p.setUniform("u_camZ", lookDir);
  p.setUniform("u_camUp", upDir);
  p.setUniform("u_camRight", rightDir);
}

void CurveNetwork::setCurveNetworkEdgeUniforms(gl::GLProgram& p) {
  p.setUniform("u_radius", radius * state::lengthScale);
}

void CurveNetwork::draw() {
  if (!enabled) {
    return;
  }

  // If there is no dominant quantity, then this class is responsible for drawing points
  if (dominantQuantity == nullptr) {

    // Ensure we have prepared buffers
    if (edgeProgram == nullptr || nodeProgram == nullptr) {
      prepare();
    }

    // Set program uniforms
    setTransformUniforms(*edgeProgram);
    setTransformUniforms(*nodeProgram);

    setCurveNetworkEdgeUniforms(*edgeProgram);
    setCurveNetworkNodeUniforms(*nodeProgram);

    edgeProgram->setUniform("u_color", baseColor);
    nodeProgram->setUniform("u_baseColor", baseColor);

    // Draw the actual curve network
    edgeProgram->draw();
    nodeProgram->draw();
  }

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }
}

void CurveNetwork::drawPick() {
  if (!enabled) {
    return;
  }

  // Ensure we have prepared buffers
  if (edgePickProgram == nullptr || nodePickProgram == nullptr) {
    preparePick();
  }

  // Set uniforms
  setTransformUniforms(*edgePickProgram);
  setTransformUniforms(*nodePickProgram);

  setCurveNetworkEdgeUniforms(*edgePickProgram);
  setCurveNetworkNodeUniforms(*nodePickProgram);

  edgePickProgram->draw();
  nodePickProgram->draw();
}

void CurveNetwork::prepare() {

  // TODO figure out billboarded cylinders to fix visual artifacs
  // This might be a starting point:
  // https://www.inf.tu-dresden.de/content/institutes/smt/cg/results/minorthesis/pbrausewetter/files/Beleg.pdf

  if (dominantQuantity != nullptr) {
    return;
  }


  // It not quantity is coloring the network, draw with a default color
  nodeProgram.reset(new gl::GLProgram(&gl::SPHERE_VERT_SHADER, &gl::SPHERE_BILLBOARD_GEOM_SHADER,
                                      &gl::SPHERE_BILLBOARD_FRAG_SHADER, gl::DrawMode::Points));
  setMaterialForProgram(*nodeProgram, "wax");

  edgeProgram.reset(new gl::GLProgram(&gl::PASSTHRU_CYLINDER_VERT_SHADER, &gl::CYLINDER_GEOM_SHADER,
                                      &gl::CYLINDER_FRAG_SHADER, gl::DrawMode::Points));
  setMaterialForProgram(*edgeProgram, "wax");

  // Fill out the geometry data for the programs
  fillNodeGeometryBuffers(*nodeProgram);
  fillEdgeGeometryBuffers(*edgeProgram);
}

void CurveNetwork::preparePick() {

  // Pick index layout (local indices):
  //   |     --- nodes ---     |      --- edges ---      |
  //   ^                       ^
  //   0                    nNodes()

  // Request pick indices
  size_t totalPickElements = nNodes() + nEdges();
  size_t pickStart = pick::requestPickBufferRange(this, totalPickElements);

  { // Set up node picking program
    nodePickProgram.reset(new gl::GLProgram(&gl::SPHERE_COLOR_VERT_SHADER, &gl::SPHERE_COLOR_BILLBOARD_GEOM_SHADER,
                                            &gl::SPHERE_COLOR_PLAIN_BILLBOARD_FRAG_SHADER, gl::DrawMode::Points));

    // Fill color buffer with packed point indices
    std::vector<glm::vec3> pickColors;
    pickColors.reserve(nNodes());
    for (size_t i = pickStart; i < pickStart + nNodes(); i++) {
      glm::vec3 val = pick::indToVec(i);
      pickColors.push_back(pick::indToVec(i));
    }


    // Store data in buffers
    nodePickProgram->setAttribute("a_color", pickColors);

    fillNodeGeometryBuffers(*nodePickProgram);
  }

  { // Set up edge picking program
    edgePickProgram.reset(new gl::GLProgram(&gl::CYLINDER_PICK_VERT_SHADER, &gl::CYLINDER_PICK_GEOM_SHADER,
                                            &gl::CYLINDER_PICK_FRAG_SHADER, gl::DrawMode::Points));

    // Fill color buffer with packed node/edge indices
    std::vector<glm::vec3> edgePickTail(nEdges());
    std::vector<glm::vec3> edgePickTip(nEdges());
    std::vector<glm::vec3> edgePickEdge(nEdges());

    // Fill posiiton and pick index buffers
    for (size_t iE = 0; iE < nEdges(); iE++) {
      auto& edge = edges[iE];
      size_t eTail = std::get<0>(edge);
      size_t eTip = std::get<1>(edge);

      glm::vec3 colorValTail = pick::indToVec(pickStart + eTail);
      glm::vec3 colorValTip = pick::indToVec(pickStart + eTip);
      glm::vec3 colorValEdge = pick::indToVec(pickStart + nNodes() + iE);
      edgePickTail[iE] = colorValTail;
      edgePickTip[iE] = colorValTip;
      edgePickEdge[iE] = colorValEdge;
    }
    edgePickProgram->setAttribute("a_color_tail", edgePickTail);
    edgePickProgram->setAttribute("a_color_tip", edgePickTip);
    edgePickProgram->setAttribute("a_color_edge", edgePickEdge);

    fillEdgeGeometryBuffers(*edgePickProgram);
  }
}

void CurveNetwork::fillNodeGeometryBuffers(gl::GLProgram& program) { program.setAttribute("a_position", nodes); }

void CurveNetwork::fillEdgeGeometryBuffers(gl::GLProgram& program) {

  // Positions at either end of edges
  std::vector<glm::vec3> posTail(nEdges());
  std::vector<glm::vec3> posTip(nEdges());
  for (size_t iE = 0; iE < nEdges(); iE++) {
    auto& edge = edges[iE];
    size_t eTail = std::get<0>(edge);
    size_t eTip = std::get<1>(edge);
    posTail[iE] = nodes[eTail];
    posTip[iE] = nodes[eTip];
  }
  program.setAttribute("a_position_tail", posTail);
  program.setAttribute("a_position_tip", posTip);
}


void CurveNetwork::buildPickUI(size_t localPickID) {

  if (localPickID < nNodes()) {
    buildNodePickUI(localPickID);
  } else if (localPickID < nNodes() + nEdges()) {
    buildEdgePickUI(localPickID - nNodes());
  } else {
    error("Bad pick index in curve network");
  }
}

void CurveNetwork::buildNodePickUI(size_t nodeInd) {

  ImGui::TextUnformatted(("node #" + std::to_string(nodeInd) + "  ").c_str());
  ImGui::SameLine();
  ImGui::TextUnformatted(to_string(nodes[nodeInd]).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildNodeInfoGUI(nodeInd);
  }

  ImGui::Indent(-20.);
}

void CurveNetwork::buildEdgePickUI(size_t edgeInd) {
  ImGui::TextUnformatted(("edge #" + std::to_string(edgeInd) + "  ").c_str());
  ImGui::SameLine();
  size_t n0 = std::get<0>(edges[edgeInd]);
  size_t n1 = std::get<1>(edges[edgeInd]);
  ImGui::TextUnformatted(("  " + std::to_string(n0) + " -- " + std::to_string(n1)).c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Indent(20.);

  // Build GUI to show the quantities
  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 3);
  for (auto& x : quantities) {
    x.second->buildEdgeInfoGUI(edgeInd);
  }

  ImGui::Indent(-20.);
}


void CurveNetwork::buildCustomUI() {
  ImGui::Text("nodes: %lld  edges: %lld", static_cast<long long int>(nNodes()), static_cast<long long int>(nEdges()));
  ImGui::ColorEdit3("Color", (float*)&baseColor, ImGuiColorEditFlags_NoInputs);
  ImGui::SameLine();
  ImGui::PushItemWidth(100);
  ImGui::SliderFloat("Radius", &radius, 0.0, .1, "%.5f", 3.);
  ImGui::PopItemWidth();
}

double CurveNetwork::lengthScale() {
  // TODO cache

  // Measure length scale as twice the radius from the center of the bounding box
  auto bound = boundingBox();
  glm::vec3 center = 0.5f * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (glm::vec3& p : nodes) {
    lengthScale = std::max(lengthScale, (double)glm::length2(p - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<glm::vec3, glm::vec3> CurveNetwork::boundingBox() {

  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();

  for (glm::vec3& rawP : nodes) {
    glm::vec3 p = glm::vec3(objectTransform * glm::vec4(rawP, 1.0));
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }

  return std::make_tuple(min, max);
}

std::string CurveNetwork::typeName() { return structureTypeName; }

// === Quantities

CurveNetworkQuantity::CurveNetworkQuantity(std::string name_, CurveNetwork& curveNetwork_, bool dominates_)
    : Quantity<CurveNetwork>(name_, curveNetwork_, dominates_) {}


void CurveNetworkQuantity::buildNodeInfoGUI(size_t nodeInd) {}
void CurveNetworkQuantity::buildEdgeInfoGUI(size_t edgeInd) {}

// === Quantity adders


CurveNetworkNodeColorQuantity* CurveNetwork::addNodeColorQuantityImpl(std::string name,
                                                                      const std::vector<glm::vec3>& colors) {
  CurveNetworkNodeColorQuantity* q = new CurveNetworkNodeColorQuantity(name, colors, *this);
  addQuantity(q);
  return q;
}

CurveNetworkEdgeColorQuantity* CurveNetwork::addEdgeColorQuantityImpl(std::string name,
                                                                      const std::vector<glm::vec3>& colors) {
  CurveNetworkEdgeColorQuantity* q = new CurveNetworkEdgeColorQuantity(name, colors, *this);
  addQuantity(q);
  return q;
}


CurveNetworkNodeScalarQuantity*
CurveNetwork::addNodeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type) {
  CurveNetworkNodeScalarQuantity* q = new CurveNetworkNodeScalarQuantity(name, data, *this, type);
  addQuantity(q);
  return q;
}

CurveNetworkEdgeScalarQuantity*
CurveNetwork::addEdgeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type) {
  CurveNetworkEdgeScalarQuantity* q = new CurveNetworkEdgeScalarQuantity(name, data, *this, type);
  addQuantity(q);
  return q;
}

CurveNetworkNodeVectorQuantity* CurveNetwork::addNodeVectorQuantityImpl(std::string name,
                                                                        const std::vector<glm::vec3>& vectors,
                                                                        VectorType vectorType) {
  CurveNetworkNodeVectorQuantity* q = new CurveNetworkNodeVectorQuantity(name, vectors, *this, vectorType);
  addQuantity(q);
  return q;
}

CurveNetworkEdgeVectorQuantity* CurveNetwork::addEdgeVectorQuantityImpl(std::string name,
                                                                        const std::vector<glm::vec3>& vectors,
                                                                        VectorType vectorType) {
  CurveNetworkEdgeVectorQuantity* q = new CurveNetworkEdgeVectorQuantity(name, vectors, *this, vectorType);
  addQuantity(q);
  return q;
}


} // namespace polyscope
