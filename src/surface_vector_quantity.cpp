// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/trace_vector_field.h"

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
    : SurfaceVectorQuantity(name, mesh_, MeshElement::VERTEX), VectorQuantity<SurfaceVertexVectorQuantity>(
                                                                   *this, vectors_, parent.vertexPositions,
                                                                   vectorType_) {
  refresh();
}

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
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE), VectorQuantity<SurfaceFaceVectorQuantity>(
                                                                 *this, vectors_, parent.faceCenters, vectorType_) {
  refresh();
}

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
// ==========         Ribbon Interface           ==========
// ========================================================

SurfaceRibbonInterface::SurfaceRibbonInterface(std::string uniqueName)
    : ribbonEnabled(uniqueName + "#ribbonEnabled", false) {}


SurfaceRibbonInterface* SurfaceRibbonInterface::setRibbonEnabled(bool newVal) {
  ribbonEnabled = newVal;
  requestRedraw();
  return this;
}
bool SurfaceRibbonInterface::isRibbonEnabled() { return ribbonEnabled.get(); }

SurfaceRibbonInterface* SurfaceRibbonInterface::setRibbonWidth(double val, bool isRelative) {
  if (ribbonArtist) {
    ribbonArtist->setWidth(val, isRelative);
    requestRedraw();
  }
  return this;
}
double SurfaceRibbonInterface::getRibbonWidth() {
  if (ribbonArtist) {
    return ribbonArtist->getWidth();
  }
  return -1;
}

SurfaceRibbonInterface* SurfaceRibbonInterface::setRibbonMaterial(std::string name) {
  if (ribbonArtist) {
    ribbonArtist->setMaterial(name);
    requestRedraw();
  }
  return this;
}
std::string SurfaceRibbonInterface::getRibbonMaterial() {
  if (ribbonArtist) {
    return ribbonArtist->getMaterial();
  }
  return "";
}


// ========================================================
// ==========        Intrinsic Face Vector       ==========
// ========================================================


SurfaceFaceIntrinsicVectorQuantity::SurfaceFaceIntrinsicVectorQuantity(std::string name,
                                                                       std::vector<glm::vec2> vectors_,
                                                                       SurfaceMesh& mesh_, VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE), TangentVectorQuantity<SurfaceFaceIntrinsicVectorQuantity>(
                                                                 *this, vectors_, parent.faceCenters,
                                                                 parent.faceTangentSpaces, vectorType_),
      SurfaceRibbonInterface(parent.uniquePrefix() + "#" + name) {
  parent.checkHaveFaceTangentSpaces();
  refresh(); // TODO
}

void SurfaceFaceIntrinsicVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void SurfaceFaceIntrinsicVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();

  if (ribbonEnabled.get() && isEnabled()) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {
      // Warning: expensive... Creates noticeable UI lag
      tangentVectors.ensureHostBufferPopulated();
      ribbonArtist.reset(new RibbonArtist(parent, traceField(parent, false, tangentVectors.data, 1, 2500)));
    }

    // Update transform matrix from parent
    ribbonArtist->draw();
  }
}

void SurfaceFaceIntrinsicVectorQuantity::buildCustomUI() {
  buildVectorUI();

  if (ImGui::Checkbox("Draw ribbon", &ribbonEnabled.get())) setRibbonEnabled(isRibbonEnabled());
  if (ribbonEnabled.get() && ribbonArtist != nullptr) {
    ImGui::SameLine();
    ribbonArtist->buildParametersGUI();
  }
}

void SurfaceFaceIntrinsicVectorQuantity::buildFaceInfoGUI(size_t iF) {
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

std::string SurfaceFaceIntrinsicVectorQuantity::niceName() { return name + " (face intrinsic vector)"; }

// ========================================================
// ==========       Intrinsic Vertex Vector      ==========
// ========================================================


SurfaceVertexIntrinsicVectorQuantity::SurfaceVertexIntrinsicVectorQuantity(std::string name,
                                                                           std::vector<glm::vec2> vectors_,
                                                                           SurfaceMesh& mesh_, VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::VERTEX),
      TangentVectorQuantity<SurfaceVertexIntrinsicVectorQuantity>(*this, vectors_, parent.vertexPositions,
                                                                  parent.vertexTangentSpaces, vectorType_),
      SurfaceRibbonInterface(parent.uniquePrefix() + "#" + name) {

  parent.checkHaveVertexTangentSpaces();
  refresh(); // TODO
}

void SurfaceVertexIntrinsicVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void SurfaceVertexIntrinsicVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();

  if (isEnabled() && ribbonEnabled.get()) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {

      // Remap to center of each face (extrinsically)

      tangentVectors.ensureHostBufferPopulated();
      parent.defaultFaceTangentSpaces.ensureHostBufferPopulated();
      parent.vertexTangentSpaces.ensureHostBufferPopulated();

      std::vector<glm::vec2> unitFaceVecs(parent.nFaces());

      for (size_t iF = 0; iF < parent.nFaces(); iF++) {
        size_t start = parent.faceIndsStart[iF];
        size_t D = parent.faceIndsStart[iF + 1] - parent.faceIndsStart[iF];

        glm::vec3 faceBasisX = parent.defaultFaceTangentSpaces.data[iF][0];
        glm::vec3 faceBasisY = parent.defaultFaceTangentSpaces.data[iF][1];

        glm::vec2 sum{0.0, 0.0};

        for (size_t j = 0; j < D; j++) {
          size_t iV = parent.faceIndsEntries[start + j];

          glm::vec2 vertVec = tangentVectors.data[iV];
          // Complex angle = std::pow(Complex(vertVec.x, vertVec.y), 1.0 / nSym);
          // vertVec = glm::vec2{angle.real(), angle.imag()};
          glm::vec3 vertexBasisX = parent.vertexTangentSpaces.data[iV][0];
          glm::vec3 vertexBasisY = parent.vertexTangentSpaces.data[iV][1];

          // Rotate in to the basis of the face
          glm::vec2 faceVecRot = rotateToTangentBasis(vertVec, vertexBasisX, vertexBasisY, faceBasisX, faceBasisY);
          // angle = std::pow(Complex(faceVec.x, faceVec.y), nSym);
          // faceVec = glm::vec2{angle.real(), angle.imag()};
          sum += faceVecRot;
        }

        unitFaceVecs[iF] = glm::normalize(sum);
      }

      // Warning: expensive... Creates noticeable UI lag
      ribbonArtist.reset(new RibbonArtist(parent, traceField(parent, true, unitFaceVecs, 1, 2500)));
    }

    // Update transform matrix from parent
    ribbonArtist->draw();
  }
}

void SurfaceVertexIntrinsicVectorQuantity::buildCustomUI() {
  buildVectorUI();

  if (ImGui::Checkbox("Draw ribbon", &ribbonEnabled.get())) setRibbonEnabled(isRibbonEnabled());
  if (ribbonEnabled.get() && ribbonArtist != nullptr) {
    ImGui::SameLine();
    ribbonArtist->buildParametersGUI();
  }
}


void SurfaceVertexIntrinsicVectorQuantity::buildVertexInfoGUI(size_t iV) {
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

std::string SurfaceVertexIntrinsicVectorQuantity::niceName() { return name + " (vertex intrinsic vector)"; }

// ========================================================
// ==========        Intrinsic One Form          ==========
// ========================================================

namespace {
// helper function used below

std::vector<glm::vec2> oneFormToFaceTangentVectors(SurfaceMesh& mesh, const std::vector<double>& oneForm,
                                                   std::vector<char>& canonicalOrientation) {

  mesh.vertexPositions.ensureHostBufferPopulated();
  mesh.faceAreas.ensureHostBufferPopulated();
  mesh.faceNormals.ensureHostBufferPopulated();
  mesh.defaultFaceTangentSpaces.ensureHostBufferPopulated();
  mesh.triangleAllEdgeInds.ensureHostBufferPopulated();
  mesh.triangleFaceInds.ensureHostBufferPopulated();

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

    glm::vec2 approxVec{glm::dot(result, mesh.defaultFaceTangentSpaces.data[iF][0]),
                        glm::dot(result, mesh.defaultFaceTangentSpaces.data[iF][1])};
    mappedVectorField[iF] = approxVec;
  }

  return mappedVectorField;
}

} // namespace

SurfaceOneFormIntrinsicVectorQuantity::SurfaceOneFormIntrinsicVectorQuantity(std::string name,
                                                                             std::vector<double> oneForm_,
                                                                             std::vector<char> canonicalOrientation_,
                                                                             SurfaceMesh& mesh_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE),
      TangentVectorQuantity<SurfaceOneFormIntrinsicVectorQuantity>(
          *this, oneFormToFaceTangentVectors(mesh_, oneForm_, canonicalOrientation_), parent.faceCenters,
          parent.defaultFaceTangentSpaces, VectorType::STANDARD),
      SurfaceRibbonInterface(parent.uniquePrefix() + "#" + name), oneForm(oneForm_),
      canonicalOrientation(canonicalOrientation_) {
  refresh(); // TODO
}

void SurfaceOneFormIntrinsicVectorQuantity::refresh() {
  // Remap to faces
  tangentVectors.data = oneFormToFaceTangentVectors(parent, oneForm, canonicalOrientation);
  tangentVectors.markHostBufferUpdated();
}

void SurfaceOneFormIntrinsicVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();

  if (isEnabled() && ribbonEnabled.get()) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {
      tangentVectors.ensureHostBufferPopulated();

      std::vector<glm::vec2> unitMappedField(parent.nFaces());
      for (size_t iF = 0; iF < parent.nFaces(); iF++) {
        unitMappedField[iF] = glm::normalize(tangentVectors.data[iF]);
      }
      // Warning: expensive... Creates noticeable UI lag
      ribbonArtist.reset(new RibbonArtist(parent, traceField(parent, true, unitMappedField, 1, 2500)));
    }


    // Update transform matrix from parent
    ribbonArtist->draw();
  }
}

void SurfaceOneFormIntrinsicVectorQuantity::buildCustomUI() {
  buildVectorUI();

  if (ImGui::Checkbox("Draw ribbon", &ribbonEnabled.get())) setRibbonEnabled(isRibbonEnabled());
  if (ribbonEnabled.get() && ribbonArtist != nullptr) {
    ImGui::SameLine();
    ribbonArtist->buildParametersGUI();
  }
}

void SurfaceOneFormIntrinsicVectorQuantity::buildEdgeInfoGUI(size_t iE) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  ImGui::Text("%g", oneForm[iE]);

  ImGui::NextColumn();
}

std::string SurfaceOneFormIntrinsicVectorQuantity::niceName() { return name + " (1-form intrinsic vector)"; }

} // namespace polyscope
