// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/curve_network.h"

#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

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
    : QuantityStructure<CurveNetwork>(name, typeName()), nodes(std::move(nodes_)), edges(std::move(edges_)),
      color(uniquePrefix() + "#color", getNextUniqueColor()), radius(uniquePrefix() + "#radius", relativeValue(0.005)),
      material(uniquePrefix() + "#material", "clay")

{

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
                       " nodes.");
    }

    // Increment degree
    nodeDegrees[nA]++;
    nodeDegrees[nB]++;
  }
}


// Helper to set uniforms
void CurveNetwork::setCurveNetworkNodeUniforms(render::ShaderProgram& p) {
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  p.setUniform("u_viewport", render::engine->getCurrentViewport());
  p.setUniform("u_pointRadius", getRadius());
}

void CurveNetwork::setCurveNetworkEdgeUniforms(render::ShaderProgram& p) {
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  p.setUniform("u_viewport", render::engine->getCurrentViewport());
  p.setUniform("u_radius", getRadius());
}

void CurveNetwork::draw() {
  if (!isEnabled()) {
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

    edgeProgram->setUniform("u_baseColor", getColor());
    nodeProgram->setUniform("u_baseColor", getColor());

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
  if (!isEnabled()) {
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
  if (dominantQuantity != nullptr) {
    return;
  }

  // It no quantity is coloring the network, draw with a default color
  nodeProgram = render::engine->requestShader("RAYCAST_SPHERE", {"SHADE_BASECOLOR"});
  render::engine->setMaterial(*nodeProgram, getMaterial());

  edgeProgram = render::engine->requestShader("RAYCAST_CYLINDER", {"SHADE_BASECOLOR"});
  render::engine->setMaterial(*edgeProgram, getMaterial());

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
    nodePickProgram = render::engine->requestShader("RAYCAST_SPHERE", {"SPHERE_PROPAGATE_COLOR"},
                                                    render::ShaderReplacementDefaults::Pick);

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
    edgePickProgram = render::engine->requestShader("RAYCAST_CYLINDER", {"CYLINDER_PROPAGATE_PICK"},
                                                    render::ShaderReplacementDefaults::Pick);

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

void CurveNetwork::fillNodeGeometryBuffers(render::ShaderProgram& program) {
  program.setAttribute("a_position", nodes);
}

void CurveNetwork::fillEdgeGeometryBuffers(render::ShaderProgram& program) {

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

void CurveNetwork::refresh() {
  nodeProgram.reset();
  edgeProgram.reset();
  nodePickProgram.reset();
  edgePickProgram.reset();
  requestRedraw();
  QuantityStructure<CurveNetwork>::refresh(); // call base class version, which refreshes quantities
}

void CurveNetwork::geometryChanged() {
  refresh();
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
  if (ImGui::ColorEdit3("Color", &color.get()[0], ImGuiColorEditFlags_NoInputs)) {
    setColor(getColor());
  }
  ImGui::SameLine();
  ImGui::PushItemWidth(100);
  if (ImGui::SliderFloat("Radius", radius.get().getValuePtr(), 0.0, .1, "%.5f", 3.)) {
    radius.manuallyChanged();
    requestRedraw();
  }
  ImGui::PopItemWidth();
}

void CurveNetwork::buildCustomOptionsUI() {
  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get()); // trigger the other updates that happen on set()
  }
}

double CurveNetwork::lengthScale() {
  // TODO cache

  // Measure length scale as twice the radius from the center of the bounding box
  auto bound = boundingBox();
  glm::vec3 center = 0.5f * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (glm::vec3& rawP : nodes) {
    glm::vec3 p = glm::vec3(objectTransform.get() * glm::vec4(rawP, 1.0));
    lengthScale = std::max(lengthScale, (double)glm::length2(p - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<glm::vec3, glm::vec3> CurveNetwork::boundingBox() {

  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();

  for (glm::vec3& rawP : nodes) {
    glm::vec3 p = glm::vec3(objectTransform.get() * glm::vec4(rawP, 1.0));
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }

  return std::make_tuple(min, max);
}

CurveNetwork* CurveNetwork::setColor(glm::vec3 newVal) {
  color = newVal;
  polyscope::requestRedraw();
  return this;
}
glm::vec3 CurveNetwork::getColor() { return color.get(); }

CurveNetwork* CurveNetwork::setRadius(float newVal, bool isRelative) {
  radius = ScaledValue<float>(newVal, isRelative);
  polyscope::requestRedraw();
  return this;
}
float CurveNetwork::getRadius() { return radius.get().asAbsolute(); }

CurveNetwork* CurveNetwork::setMaterial(std::string m) {
  material = m;
  geometryChanged(); // (serves the purpose of re-initializing everything, though this is a bit overkill)
  requestRedraw();
  return this;
}
std::string CurveNetwork::getMaterial() { return material.get(); }

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
