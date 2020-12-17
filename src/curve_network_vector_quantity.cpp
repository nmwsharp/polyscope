// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/curve_network_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <complex>
#include <fstream>
#include <iostream>

using std::cout;
using std::endl;

namespace polyscope {

CurveNetworkVectorQuantity::CurveNetworkVectorQuantity(std::string name, CurveNetwork& network_, VectorType vectorType_)
    : CurveNetworkQuantity(name, network_), vectorType(vectorType_) {}

void CurveNetworkVectorQuantity::prepareVectorArtist() {
  vectorArtist.reset(new VectorArtist(parent, name + "#vectorartist", vectorRoots, vectors, vectorType));
}

void CurveNetworkVectorQuantity::draw() {
  if (!isEnabled()) return;
  vectorArtist->draw();
}

void CurveNetworkVectorQuantity::buildCustomUI() {
  ImGui::SameLine();
  vectorArtist->buildParametersUI();
  drawSubUI();
}

void CurveNetworkVectorQuantity::drawSubUI() {}

CurveNetworkVectorQuantity* CurveNetworkVectorQuantity::setVectorLengthScale(double newLength, bool isRelative) {
  vectorArtist->setVectorLengthScale(newLength, isRelative);
  return this;
}
double CurveNetworkVectorQuantity::getVectorLengthScale() { return vectorArtist->getVectorLengthScale(); }
CurveNetworkVectorQuantity* CurveNetworkVectorQuantity::setVectorRadius(double val, bool isRelative) {
  vectorArtist->setVectorRadius(val, isRelative);
  return this;
}
double CurveNetworkVectorQuantity::getVectorRadius() { return vectorArtist->getVectorRadius(); }
CurveNetworkVectorQuantity* CurveNetworkVectorQuantity::setVectorColor(glm::vec3 color) {
  vectorArtist->setVectorColor(color);
  return this;
}
glm::vec3 CurveNetworkVectorQuantity::getVectorColor() { return vectorArtist->getVectorColor(); }

CurveNetworkVectorQuantity* CurveNetworkVectorQuantity::setMaterial(std::string m) {
  vectorArtist->setMaterial(m);
  return this;
}
std::string CurveNetworkVectorQuantity::getMaterial() { return vectorArtist->getMaterial(); }


std::string CurveNetworkEdgeVectorQuantity::niceName() { return name + " (edge vector)"; }

// ========================================================
// ==========           Node Vector            ==========
// ========================================================

CurveNetworkNodeVectorQuantity::CurveNetworkNodeVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                               CurveNetwork& network_, VectorType vectorType_)

    : CurveNetworkVectorQuantity(name, network_, vectorType_) {
  vectors = vectors_;
  refresh();
}

void CurveNetworkNodeVectorQuantity::refresh() {
  size_t i = 0;
  vectorRoots = parent.nodes;

  prepareVectorArtist();
  Quantity::refresh();
}

void CurveNetworkNodeVectorQuantity::buildNodeInfoGUI(size_t iV) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectors[iV];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vectors[iV]));
  ImGui::NextColumn();
}

std::string CurveNetworkNodeVectorQuantity::niceName() { return name + " (node vector)"; }

// ========================================================
// ==========            Edge Vector             ==========
// ========================================================

CurveNetworkEdgeVectorQuantity::CurveNetworkEdgeVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                               CurveNetwork& network_, VectorType vectorType_)
    : CurveNetworkVectorQuantity(name, network_, vectorType_) {
  vectors = vectors_;
  refresh();
}

void CurveNetworkEdgeVectorQuantity::refresh() {
  // Copy the vectors
  vectorRoots.resize(parent.nEdges());

  for (size_t iE = 0; iE < parent.nEdges(); iE++) {
    auto& edge = parent.edges[iE];
    size_t eTail = std::get<0>(edge);
    size_t eTip = std::get<1>(edge);

    vectorRoots[iE] = 0.5f * (parent.nodes[eTail] + parent.nodes[eTip]);
  }

  prepareVectorArtist();
  Quantity::refresh();
}

void CurveNetworkEdgeVectorQuantity::buildEdgeInfoGUI(size_t iF) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectors[iF];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vectors[iF]));
  ImGui::NextColumn();
}

} // namespace polyscope
