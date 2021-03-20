// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/volume_mesh_vector_quantity.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

namespace polyscope {

VolumeMeshVectorQuantity::VolumeMeshVectorQuantity(std::string name, VolumeMesh& mesh_, VolumeMeshElement definedOn_,
                                                   VectorType vectorType_)
    : VolumeMeshQuantity(name, mesh_), vectorType(vectorType_) {}


void VolumeMeshVectorQuantity::prepareVectorArtist() {
  vectorArtist.reset(new VectorArtist(parent, name + "#vectorartist", vectorRoots, vectors, vectorType));
}

void VolumeMeshVectorQuantity::draw() {
  if (!isEnabled()) return;
  vectorArtist->draw();
}

void VolumeMeshVectorQuantity::buildCustomUI() {
  ImGui::SameLine();
  vectorArtist->buildParametersUI();
  drawSubUI();
}

void VolumeMeshVectorQuantity::drawSubUI() {}

VolumeMeshVectorQuantity* VolumeMeshVectorQuantity::setVectorLengthScale(double newLength, bool isRelative) {
  vectorArtist->setVectorLengthScale(newLength, isRelative);
  return this;
}
double VolumeMeshVectorQuantity::getVectorLengthScale() { return vectorArtist->getVectorLengthScale(); }
VolumeMeshVectorQuantity* VolumeMeshVectorQuantity::setVectorRadius(double val, bool isRelative) {
  vectorArtist->setVectorRadius(val, isRelative);
  return this;
}
double VolumeMeshVectorQuantity::getVectorRadius() { return vectorArtist->getVectorRadius(); }
VolumeMeshVectorQuantity* VolumeMeshVectorQuantity::setVectorColor(glm::vec3 color) {
  vectorArtist->setVectorColor(color);
  return this;
}
glm::vec3 VolumeMeshVectorQuantity::getVectorColor() { return vectorArtist->getVectorColor(); }

VolumeMeshVectorQuantity* VolumeMeshVectorQuantity::setMaterial(std::string m) {
  vectorArtist->setMaterial(m);
  return this;
}
std::string VolumeMeshVectorQuantity::getMaterial() { return vectorArtist->getMaterial(); }

// ========================================================
// ==========           Vertex Vector            ==========
// ========================================================

VolumeMeshVertexVectorQuantity::VolumeMeshVertexVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                               VolumeMesh& mesh_, VectorType vectorType_)

    : VolumeMeshVectorQuantity(name, mesh_, VolumeMeshElement::VERTEX, vectorType_) {
  vectors = vectors_;
  refresh();
}

void VolumeMeshVertexVectorQuantity::refresh() {
  vectorRoots = parent.vertices;
  prepareVectorArtist();
}

void VolumeMeshVertexVectorQuantity::buildVertexInfoGUI(size_t iV) {
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

std::string VolumeMeshVertexVectorQuantity::niceName() { return name + " (vertex vector)"; }

// ========================================================
// ==========            Cell Vector             ==========
// ========================================================

VolumeMeshCellVectorQuantity::VolumeMeshCellVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                           VolumeMesh& mesh_, VectorType vectorType_)
    : VolumeMeshVectorQuantity(name, mesh_, VolumeMeshElement::CELL, vectorType_) {
  vectors = vectors_;
  refresh();
}

void VolumeMeshCellVectorQuantity::refresh() {
  vectorRoots.resize(parent.nCells());
  for (size_t iC = 0; iC < parent.nCells(); iC++) {
    vectorRoots[iC] = parent.cellCenter(iC);
  }
  prepareVectorArtist();
}

void VolumeMeshCellVectorQuantity::buildCellInfoGUI(size_t iF) {
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

std::string VolumeMeshCellVectorQuantity::niceName() { return name + " (cell vector)"; }

} // namespace polyscope
