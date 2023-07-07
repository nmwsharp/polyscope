// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/surface_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

#include <complex>
#include <fstream>
#include <iostream>

namespace polyscope {

SurfaceVectorQuantity::SurfaceVectorQuantity(std::string name, SurfaceMesh& mesh_, MeshElement definedOn_)
    : SurfaceMeshQuantity(name, mesh_) {}


// ========================================================
// ==========           Vertex Vector            ==========
// ========================================================

SurfaceVertexVectorQuantity::SurfaceVertexVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                         SurfaceMesh& mesh_, VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::VERTEX),
      VectorQuantity<SurfaceVertexVectorQuantity>(*this, vectors_, parent.vertexPositions, vectorType_) {}

void SurfaceVertexVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void SurfaceVertexVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();
}

void SurfaceVertexVectorQuantity::buildCustomUI() { buildVectorUI(); }


void SurfaceVertexVectorQuantity::buildVertexInfoGUI(size_t iV) {
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

std::string SurfaceVertexVectorQuantity::niceName() { return name + " (vertex vector)"; }

// ========================================================
// ==========            Face Vector             ==========
// ========================================================

SurfaceFaceVectorQuantity::SurfaceFaceVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                     SurfaceMesh& mesh_, VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE),
      VectorQuantity<SurfaceFaceVectorQuantity>(*this, vectors_, parent.faceCenters, vectorType_) {}

void SurfaceFaceVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void SurfaceFaceVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();
}

void SurfaceFaceVectorQuantity::buildCustomUI() { buildVectorUI(); }

void SurfaceFaceVectorQuantity::buildFaceInfoGUI(size_t iF) {
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

std::string SurfaceFaceVectorQuantity::niceName() { return name + " (face vector)"; }


// ========================================================
// ==========        Tangent Face Vector       ==========
// ========================================================


SurfaceFaceTangentVectorQuantity::SurfaceFaceTangentVectorQuantity(std::string name, std::vector<glm::vec2> vectors_,
                                                                   std::vector<glm::vec3> basisX_,
                                                                   std::vector<glm::vec3> basisY_, SurfaceMesh& mesh_,
                                                                   int nSym_, VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE),
      TangentVectorQuantity<SurfaceFaceTangentVectorQuantity>(*this, vectors_, basisX_, basisY_, parent.faceCenters,
                                                              nSym_, vectorType_) {}

void SurfaceFaceTangentVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void SurfaceFaceTangentVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();
}

void SurfaceFaceTangentVectorQuantity::buildCustomUI() { buildVectorUI(); }

void SurfaceFaceTangentVectorQuantity::buildFaceInfoGUI(size_t iF) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec2 vec = tangentVectors.getValue(iF);

  std::stringstream buffer;
  buffer << vec;
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vec));
  ImGui::NextColumn();
}

std::string SurfaceFaceTangentVectorQuantity::niceName() {
  if (nSym == 1) {
    return name + " (face tangent vector)";
  } else {
    return name + " (face tangent vector sym=" + std::to_string(nSym) + ")";
  }
}

// ========================================================
// ==========       Tangent Vertex Vector      ==========
// ========================================================


SurfaceVertexTangentVectorQuantity::SurfaceVertexTangentVectorQuantity(
    std::string name, std::vector<glm::vec2> vectors_, std::vector<glm::vec3> basisX_, std::vector<glm::vec3> basisY_,
    SurfaceMesh& mesh_, int nSym_, VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::VERTEX),
      TangentVectorQuantity<SurfaceVertexTangentVectorQuantity>(*this, vectors_, basisX_, basisY_,
                                                                parent.vertexPositions, nSym_, vectorType_) {}

void SurfaceVertexTangentVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void SurfaceVertexTangentVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();
}

void SurfaceVertexTangentVectorQuantity::buildCustomUI() { buildVectorUI(); }


void SurfaceVertexTangentVectorQuantity::buildVertexInfoGUI(size_t iV) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec2 vec = tangentVectors.getValue(iV);

  std::stringstream buffer;
  buffer << vec;
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vec));
  ImGui::NextColumn();
}

std::string SurfaceVertexTangentVectorQuantity::niceName() {
  if (nSym == 1) {
    return name + " (vertex tangent vector)";
  } else {
    return name + " (vertex tangent vector sym=" + std::to_string(nSym) + ")";
  }
}

// ========================================================
// ==========        Tangent One Form          ============
// ========================================================

namespace {
// helper function used below

std::vector<glm::vec2> oneFormToFaceTangentVectors(SurfaceMesh& mesh, const std::vector<double>& oneForm,
                                                   std::vector<char>& canonicalOrientation) {

  mesh.vertexPositions.ensureHostBufferPopulated();
  mesh.faceAreas.ensureHostBufferPopulated();
  mesh.faceNormals.ensureHostBufferPopulated();
  mesh.defaultFaceTangentBasisX.ensureHostBufferPopulated();
  mesh.defaultFaceTangentBasisY.ensureHostBufferPopulated();
  mesh.triangleAllEdgeInds.ensureHostBufferPopulated();

  std::vector<glm::vec2> mappedVectorField(mesh.nFaces());

  for (size_t iF = 0; iF < mesh.nFaces(); iF++) {

    std::array<float, 3> formValues;
    std::array<glm::vec3, 3> vecValues;
    for (size_t j = 0; j < 3; j++) {
      size_t vA = mesh.triangleVertexInds.data[3 * iF + j];
      size_t vB = mesh.triangleVertexInds.data[3 * iF + ((j + 1) % 3)];
      size_t iE = mesh.triangleAllEdgeInds.data[9 * iF + j];

      bool isCanonicalOriented = (vB > vA) != (canonicalOrientation[iE]); // TODO double check convention
      double orientationSign = isCanonicalOriented ? 1. : -1.;

      formValues[j] = orientationSign * oneForm[iE];

      glm::vec3 heVec = mesh.vertexPositions.data[vB] - mesh.vertexPositions.data[vA];
      vecValues[j] = glm::cross(heVec, mesh.faceNormals.data[iF]);
    }

    // Whitney interpolation at center
    glm::vec3 result{0., 0., 0.};
    for (int j = 0; j < 3; j++) {
      result += (formValues[(j + 1) % 3] - formValues[(j + 2) % 3]) * vecValues[j];
    }
    result /= static_cast<float>(6. * mesh.faceAreas.data[iF]);

    glm::vec2 approxVec{glm::dot(result, mesh.defaultFaceTangentBasisX.data[iF]),
                        glm::dot(result, mesh.defaultFaceTangentBasisY.data[iF])};
    mappedVectorField[iF] = approxVec;
  }

  return mappedVectorField;
}

} // namespace

SurfaceOneFormTangentVectorQuantity::SurfaceOneFormTangentVectorQuantity(std::string name, std::vector<double> oneForm_,
                                                                         std::vector<char> canonicalOrientation_,
                                                                         SurfaceMesh& mesh_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE),
      TangentVectorQuantity<SurfaceOneFormTangentVectorQuantity>(
          *this, oneFormToFaceTangentVectors(mesh_, oneForm_, canonicalOrientation_),
          mesh_.defaultFaceTangentBasisX.getPopulatedHostBufferRef(),
          mesh_.defaultFaceTangentBasisY.getPopulatedHostBufferRef(), parent.faceCenters, 1, VectorType::STANDARD),
      oneForm(oneForm_), canonicalOrientation(canonicalOrientation_) {}

void SurfaceOneFormTangentVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void SurfaceOneFormTangentVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();
}

void SurfaceOneFormTangentVectorQuantity::buildCustomUI() { buildVectorUI(); }

void SurfaceOneFormTangentVectorQuantity::buildEdgeInfoGUI(size_t iE) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", oneForm[iE]);
  ImGui::NextColumn();
}

std::string SurfaceOneFormTangentVectorQuantity::niceName() { return name + " (1-form tangent vector)"; }

} // namespace polyscope
