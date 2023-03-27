// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/curve_network.h"

#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

namespace polyscope {

// Initialize statics
const std::string CurveNetwork::structureTypeName = "Curve Network";

// Constructor
CurveNetwork::CurveNetwork(std::string name, std::vector<glm::vec3> nodes_, std::vector<std::array<size_t, 2>> edges_)
    : // clang-format off
      QuantityStructure<CurveNetwork>(name, typeName()), 
      nodePositions(uniquePrefix() + "nodePositions", nodePositionsData),
      edgeTailInds(uniquePrefix() + "edgeTailInds", edgeTailIndsData),
      edgeTipInds(uniquePrefix() + "edgeTipInds", edgeTipIndsData),
      edgeCenters(uniquePrefix() + "edgeCenters", edgeCentersData, std::bind(&CurveNetwork::computeEdgeCenters, this)),         
      nodePositionsData(std::move(nodes_)), 
      color(uniquePrefix() + "#color", getNextUniqueColor()), 
      radius(uniquePrefix() + "#radius", relativeValue(0.005)),
      material(uniquePrefix() + "#material", "clay")
// clang-format on
{

  // Copy interleaved data in to tip and tails buffers below
  edgeTailIndsData.resize(edges_.size());
  edgeTipIndsData.resize(edges_.size());

  // Compute node degrees; some quantities want them for visualizations
  nodeDegrees = std::vector<size_t>(nNodes(), 0);

  size_t maxInd = nodePositions.size();
  for (size_t iE = 0; iE < edges_.size(); iE++) {
    auto edge = edges_[iE];
    size_t nA = std::get<0>(edge);
    size_t nB = std::get<1>(edge);

    edgeTailIndsData[iE] = nA;
    edgeTipIndsData[iE] = nB;

    // Make sure there are no out of bounds indices
    if (nA >= maxInd || nB >= maxInd) {
      exception("CurveNetwork [" + name + "] edge " + std::to_string(iE) + " has bad node indices { " +
                std::to_string(nA) + " , " + std::to_string(nB) + " } but there are " + std::to_string(maxInd) +
                " nodes.");
    }

    // Increment degree
    nodeDegrees[nA]++;
    nodeDegrees[nB]++;
  }

  updateObjectSpaceBounds();
}

float CurveNetwork::computeRadiusMultiplierUniform() {
  if (nodeRadiusQuantityName != "" && !nodeRadiusQuantityAutoscale) {
    // special case: ignore radius uniform
    return 1.;
  } else {
    // common case

    float scalarQScale = 1.;
    if (nodeRadiusQuantityName != "") {
      CurveNetworkNodeScalarQuantity& radQ = resolveNodeRadiusQuantity();
      scalarQScale = std::max(0., radQ.getDataRange().second);
    }

    return getRadius() / scalarQScale;
  }
}

// Helper to set uniforms
void CurveNetwork::setCurveNetworkNodeUniforms(render::ShaderProgram& p) {
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  p.setUniform("u_viewport", render::engine->getCurrentViewport());
  p.setUniform("u_pointRadius", computeRadiusMultiplierUniform());
}

void CurveNetwork::setCurveNetworkEdgeUniforms(render::ShaderProgram& p) {
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  p.setUniform("u_viewport", render::engine->getCurrentViewport());
  p.setUniform("u_radius", computeRadiusMultiplierUniform());
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
    setStructureUniforms(*edgeProgram);
    setStructureUniforms(*nodeProgram);

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
  for (auto& x : floatingQuantities) {
    x.second->draw();
  }
}

void CurveNetwork::drawDelayed() {
  if (!isEnabled()) {
    return;
  }

  for (auto& x : quantities) {
    x.second->drawDelayed();
  }
  for (auto& x : floatingQuantities) {
    x.second->drawDelayed();
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
  setStructureUniforms(*edgePickProgram);
  setStructureUniforms(*nodePickProgram);

  setCurveNetworkEdgeUniforms(*edgePickProgram);
  setCurveNetworkNodeUniforms(*nodePickProgram);

  edgePickProgram->draw();
  nodePickProgram->draw();
}

std::vector<std::string> CurveNetwork::addCurveNetworkNodeRules(std::vector<std::string> initRules) {
  initRules = addStructureRules(initRules);

  if (nodeRadiusQuantityName != "") {
    initRules.push_back("SPHERE_VARIABLE_SIZE");
  }
  if (wantsCullPosition()) {
    initRules.push_back("SPHERE_CULLPOS_FROM_CENTER");
  }
  return initRules;
}
std::vector<std::string> CurveNetwork::addCurveNetworkEdgeRules(std::vector<std::string> initRules) {
  initRules = addStructureRules(initRules);

  // use node radius to blend cylinder radius
  if (nodeRadiusQuantityName != "") {
    initRules.push_back("CYLINDER_VARIABLE_SIZE");
  }

  if (wantsCullPosition()) {
    initRules.push_back("CYLINDER_CULLPOS_FROM_MID");
  }
  return initRules;
}

void CurveNetwork::prepare() {
  if (dominantQuantity != nullptr) {
    return;
  }

  // It no quantity is coloring the network, draw with a default color

  {
    nodeProgram = render::engine->requestShader("RAYCAST_SPHERE", addCurveNetworkNodeRules({"SHADE_BASECOLOR"}));
    render::engine->setMaterial(*nodeProgram, getMaterial());
  }

  {
    edgeProgram = render::engine->requestShader("RAYCAST_CYLINDER", addCurveNetworkEdgeRules({"SHADE_BASECOLOR"}));
    render::engine->setMaterial(*edgeProgram, getMaterial());
  }

  // Fill out the geometry data for the programs
  fillNodeGeometryBuffers(*nodeProgram);
  fillEdgeGeometryBuffers(*edgeProgram);
}

void CurveNetwork::preparePick() {
  edgeTailInds.ensureHostBufferPopulated();
  edgeTipInds.ensureHostBufferPopulated();

  // Pick index layout (local indices):
  //   |     --- nodes ---     |      --- edges ---      |
  //   ^                       ^
  //   0                    nNodes()

  // Request pick indices
  size_t totalPickElements = nNodes() + nEdges();
  size_t pickStart = pick::requestPickBufferRange(this, totalPickElements);

  { // Set up node picking program
    nodePickProgram =
        render::engine->requestShader("RAYCAST_SPHERE", addCurveNetworkNodeRules({"SPHERE_PROPAGATE_COLOR"}),
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
    edgePickProgram =
        render::engine->requestShader("RAYCAST_CYLINDER", addCurveNetworkEdgeRules({"CYLINDER_PROPAGATE_PICK"}),
                                      render::ShaderReplacementDefaults::Pick);

    // Fill color buffer with packed node/edge indices
    std::vector<glm::vec3> edgePickTail(nEdges());
    std::vector<glm::vec3> edgePickTip(nEdges());
    std::vector<glm::vec3> edgePickEdge(nEdges());

    // Fill posiiton and pick index buffers
    for (size_t iE = 0; iE < nEdges(); iE++) {
      size_t eTail = edgeTailInds.data[iE];
      size_t eTip = edgeTipInds.data[iE];

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
  program.setAttribute("a_position", nodePositions.getRenderAttributeBuffer());

  if (nodeRadiusQuantityName != "") {
    CurveNetworkNodeScalarQuantity& nodeRadQ = resolveNodeRadiusQuantity();
    program.setAttribute("a_pointRadius", nodeRadQ.values.getRenderAttributeBuffer());
  }
}

void CurveNetwork::fillEdgeGeometryBuffers(render::ShaderProgram& program) {
  program.setAttribute("a_position_tail", nodePositions.getIndexedRenderAttributeBuffer(edgeTailInds));
  program.setAttribute("a_position_tip", nodePositions.getIndexedRenderAttributeBuffer(edgeTipInds));

  if (nodeRadiusQuantityName != "") {
    CurveNetworkNodeScalarQuantity& nodeRadQ = resolveNodeRadiusQuantity();
    program.setAttribute("a_tailRadius", nodeRadQ.values.getIndexedRenderAttributeBuffer(edgeTailInds));
    program.setAttribute("a_tipRadius", nodeRadQ.values.getIndexedRenderAttributeBuffer(edgeTipInds));
  }
}

void CurveNetwork::computeEdgeCenters() {
  nodePositions.ensureHostBufferPopulated();
  edgeTailInds.ensureHostBufferPopulated();
  edgeTipInds.ensureHostBufferPopulated();

  edgeCenters.data.resize(nEdges());

  for (size_t iE = 0; iE < nEdges(); iE++) {
    size_t eTail = edgeTailInds.data[iE];
    size_t eTip = edgeTipInds.data[iE];
    glm::vec3 p = 0.5f * (nodePositions.data[eTail] + nodePositions.data[eTip]);
    edgeCenters.data[iE] = p;
  }

  edgeCenters.markHostBufferUpdated();
}

void CurveNetwork::refresh() {
  recomputeGeometryIfPopulated();

  nodeProgram.reset();
  edgeProgram.reset();
  nodePickProgram.reset();
  edgePickProgram.reset();
  requestRedraw();
  QuantityStructure<CurveNetwork>::refresh(); // call base class version, which refreshes quantities
}

void CurveNetwork::recomputeGeometryIfPopulated() { edgeCenters.recomputeIfPopulated(); }

void CurveNetwork::buildPickUI(size_t localPickID) {

  if (localPickID < nNodes()) {
    buildNodePickUI(localPickID);
  } else if (localPickID < nNodes() + nEdges()) {
    buildEdgePickUI(localPickID - nNodes());
  } else {
    exception("Bad pick index in curve network");
  }
}

void CurveNetwork::buildNodePickUI(size_t nodeInd) {

  ImGui::TextUnformatted(("node #" + std::to_string(nodeInd) + "  ").c_str());
  ImGui::SameLine();
  ImGui::TextUnformatted(to_string(nodePositions.getValue(nodeInd)).c_str());

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
  size_t n0 = edgeTailInds.getValue(edgeInd);
  size_t n1 = edgeTipInds.getValue(edgeInd);
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
  if (ImGui::SliderFloat("Radius", radius.get().getValuePtr(), 0.0, .1, "%.5f",
                         ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
    radius.manuallyChanged();
    requestRedraw();
  }
  ImGui::PopItemWidth();
}

void CurveNetwork::buildCustomOptionsUI() {

  if (ImGui::BeginMenu("Variable Radius")) {

    if (ImGui::MenuItem("none", nullptr, nodeRadiusQuantityName == "")) clearNodeRadiusQuantity();
    ImGui::Separator();

    for (auto& q : quantities) {
      CurveNetworkNodeScalarQuantity* scalarQ = dynamic_cast<CurveNetworkNodeScalarQuantity*>(q.second.get());
      if (scalarQ != nullptr) {
        if (ImGui::MenuItem(scalarQ->name.c_str(), nullptr, nodeRadiusQuantityName == scalarQ->name))
          setNodeRadiusQuantity(scalarQ);
      }
    }

    ImGui::EndMenu();
  }


  if (render::buildMaterialOptionsGui(material.get())) {
    material.manuallyChanged();
    setMaterial(material.get()); // trigger the other updates that happen on set()
  }
}

void CurveNetwork::updateObjectSpaceBounds() {
  nodePositions.ensureHostBufferPopulated();

  // bounding box
  glm::vec3 min = glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  glm::vec3 max = -glm::vec3{1, 1, 1} * std::numeric_limits<float>::infinity();
  for (const glm::vec3& p : nodePositions.data) {
    min = componentwiseMin(min, p);
    max = componentwiseMax(max, p);
  }
  objectSpaceBoundingBox = std::make_tuple(min, max);

  // length scale, as twice the radius from the center of the bounding box
  glm::vec3 center = 0.5f * (min + max);
  float lengthScale = 0.0;
  for (const glm::vec3& p : nodePositions.data) {
    lengthScale = std::max(lengthScale, glm::length2(p - center));
  }
  objectSpaceLengthScale = 2 * std::sqrt(lengthScale);
}

CurveNetwork* CurveNetwork::setColor(glm::vec3 newVal) {
  color = newVal;
  polyscope::requestRedraw();
  return this;
}
glm::vec3 CurveNetwork::getColor() { return color.get(); }

void CurveNetwork::setNodeRadiusQuantity(CurveNetworkNodeScalarQuantity* quantity, bool autoScale) {
  setNodeRadiusQuantity(quantity->name, autoScale);
}

void CurveNetwork::setNodeRadiusQuantity(std::string name, bool autoScale) {
  nodeRadiusQuantityName = name;
  nodeRadiusQuantityAutoscale = autoScale;

  resolveNodeRadiusQuantity(); // do it once, just so we fail fast if it doesn't exist

  refresh();
}

void CurveNetwork::clearNodeRadiusQuantity() {
  nodeRadiusQuantityName = "";
  refresh();
};

CurveNetwork* CurveNetwork::setRadius(float newVal, bool isRelative) {
  radius = ScaledValue<float>(newVal, isRelative);
  polyscope::requestRedraw();
  return this;
}
float CurveNetwork::getRadius() { return radius.get().asAbsolute(); }

CurveNetwork* CurveNetwork::setMaterial(std::string m) {
  material = m;
  refresh(); // (serves the purpose of re-initializing everything, though this is a bit overkill)
  requestRedraw();
  return this;
}
std::string CurveNetwork::getMaterial() { return material.get(); }

std::string CurveNetwork::typeName() { return structureTypeName; }

// === Quantities

CurveNetworkQuantity::CurveNetworkQuantity(std::string name_, CurveNetwork& curveNetwork_, bool dominates_)
    : QuantityS<CurveNetwork>(name_, curveNetwork_, dominates_) {}


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

CurveNetworkNodeScalarQuantity& CurveNetwork::resolveNodeRadiusQuantity() {
  CurveNetworkNodeScalarQuantity* sizeScalarQ = nullptr;
  CurveNetworkQuantity* sizeQ = getQuantity(nodeRadiusQuantityName);
  if (sizeQ != nullptr) {
    sizeScalarQ = dynamic_cast<CurveNetworkNodeScalarQuantity*>(sizeQ);
    if (sizeScalarQ == nullptr) {
      exception("Cannot populate node size from quantity [" + name + "], it is not a scalar quantity");
    }
  } else {
    exception("Cannot populate node size from quantity [" + name + "], it does not exist");
  }

  return *sizeScalarQ;
}

} // namespace polyscope
