// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/volume_mesh_vector_quantity.h"

#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

namespace polyscope {

VolumeMeshVectorQuantity::VolumeMeshVectorQuantity(std::string name, VolumeMesh& mesh_, VolumeMeshElement definedOn_)
    : VolumeMeshQuantity(name, mesh_), definedOn(definedOn_) {}


// ========================================================
// ==========           Vertex Vector            ==========
// ========================================================

VolumeMeshVertexVectorQuantity::VolumeMeshVertexVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                               VolumeMesh& mesh_, VectorType vectorType_)

    : VolumeMeshVectorQuantity(name, mesh_, VolumeMeshElement::VERTEX),
      VectorQuantity<VolumeMeshVertexVectorQuantity>(*this, vectors_, parent.vertexPositions, vectorType_) {}

void VolumeMeshVertexVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void VolumeMeshVertexVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();
}

void VolumeMeshVertexVectorQuantity::buildCustomUI() { buildVectorUI(); }

void VolumeMeshVertexVectorQuantity::buildVertexInfoGUI(size_t iV) {
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

std::string VolumeMeshVertexVectorQuantity::niceName() { return name + " (vertex vector)"; }

// ========================================================
// ==========            Cell Vector             ==========
// ========================================================

VolumeMeshCellVectorQuantity::VolumeMeshCellVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                           VolumeMesh& mesh_, VectorType vectorType_)
    : VolumeMeshVectorQuantity(name, mesh_, VolumeMeshElement::CELL),
      VectorQuantity<VolumeMeshCellVectorQuantity>(*this, vectors_, parent.cellCenters, vectorType_) {
  refresh();
}

void VolumeMeshCellVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void VolumeMeshCellVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();
}

void VolumeMeshCellVectorQuantity::buildCustomUI() { buildVectorUI(); }

void VolumeMeshCellVectorQuantity::buildCellInfoGUI(size_t iC) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 vec = vectors.getValue(iC);

  std::stringstream buffer;
  buffer << vec;
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vec));
  ImGui::NextColumn();
}

std::string VolumeMeshCellVectorQuantity::niceName() { return name + " (cell vector)"; }

} // namespace polyscope
