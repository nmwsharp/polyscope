// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/curve_network_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <complex>
#include <fstream>
#include <iostream>

namespace polyscope {

CurveNetworkVectorQuantity::CurveNetworkVectorQuantity(std::string name, CurveNetwork& network_)
    : CurveNetworkQuantity(name, network_) {}


// ========================================================
// ==========           Node Vector            ==========
// ========================================================

CurveNetworkNodeVectorQuantity::CurveNetworkNodeVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                               CurveNetwork& network_, VectorType vectorType_)

    : CurveNetworkVectorQuantity(name, network_),
      VectorQuantity<CurveNetworkNodeVectorQuantity>(*this, vectors_, parent.nodePositions, vectorType_) {
  refresh();
}

void CurveNetworkNodeVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void CurveNetworkNodeVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();
}

void CurveNetworkNodeVectorQuantity::buildCustomUI() { buildVectorUI(); }

void CurveNetworkNodeVectorQuantity::buildNodeInfoGUI(size_t iV) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 vec = vectors.getValue(iV);

  std::stringstream buffer;
  buffer << vec;
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vec));
  ImGui::NextColumn();
}

std::string CurveNetworkNodeVectorQuantity::niceName() { return name + " (node vector)"; }

// ========================================================
// ==========            Edge Vector             ==========
// ========================================================

CurveNetworkEdgeVectorQuantity::CurveNetworkEdgeVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                               CurveNetwork& network_, VectorType vectorType_)
    : CurveNetworkVectorQuantity(name, network_),
      VectorQuantity<CurveNetworkEdgeVectorQuantity>(*this, vectors_, parent.edgeCenters, vectorType_) {
  refresh();
}

void CurveNetworkEdgeVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void CurveNetworkEdgeVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();
}

void CurveNetworkEdgeVectorQuantity::buildCustomUI() { buildVectorUI(); }

void CurveNetworkEdgeVectorQuantity::buildEdgeInfoGUI(size_t iF) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 vec = vectors.getValue(iF);

  std::stringstream buffer;
  buffer << vec;
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vec));
  ImGui::NextColumn();
}

std::string CurveNetworkEdgeVectorQuantity::niceName() { return name + " (edge vector)"; }

} // namespace polyscope
