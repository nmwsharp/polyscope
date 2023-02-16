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
    : SurfaceMeshQuantity(name, mesh_)
// ribbonEnabled(uniquePrefix() + "#ribbonEnabled", false)
{}


/*
bool SurfaceVectorQuantity::isRibbonEnabled() { return ribbonEnabled.get(); }

SurfaceVectorQuantity* SurfaceVectorQuantity::setRibbonWidth(double val, bool isRelative) {
  if (ribbonArtist) {
    ribbonArtist->setWidth(val, isRelative);
  }
  return this;
}
double SurfaceVectorQuantity::getRibbonWidth() {
  if (ribbonArtist) {
    return ribbonArtist->getWidth();
  }
  return -1;
}

SurfaceVectorQuantity* SurfaceVectorQuantity::setRibbonMaterial(std::string name) {
  if (ribbonArtist) {
    ribbonArtist->setMaterial(name);
  }
  return this;
}
std::string SurfaceVectorQuantity::getRibbonMaterial() {
  if (ribbonArtist) {
    return ribbonArtist->getMaterial();
  }
  return "";
}
*/

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
// ==========        Intrinsic Face Vector       ==========
// ========================================================


SurfaceFaceIntrinsicVectorQuantity::SurfaceFaceIntrinsicVectorQuantity(std::string name,
                                                                       std::vector<glm::vec2> vectors_,
                                                                       SurfaceMesh& mesh_, VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE), TangentVectorQuantity<SurfaceFaceIntrinsicVectorQuantity>(
                                                                 *this, vectors_, parent.faceCenters,
                                                                 parent.faceTangentSpaces, vectorType_) {
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

  /*
  if (ribbonEnabled.get() && isEnabled()) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {
      // Warning: expensive... Creates noticeable UI lag
      ribbonArtist.reset(new RibbonArtist(parent, traceField(parent, vectorField, nSym, 2500)));
    }

    // Update transform matrix from parent
    ribbonArtist->draw();
  }
  */
}

void SurfaceFaceIntrinsicVectorQuantity::buildCustomUI() {
  buildVectorUI();

  /*
  if (ImGui::Checkbox("Draw ribbon", &ribbonEnabled.get())) setRibbonEnabled(isRibbonEnabled());
  if (ribbonEnabled.get() && ribbonArtist != nullptr) {
    ImGui::SameLine();
    ribbonArtist->buildParametersGUI();
  }
  */
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
                                                                  parent.vertexTangentSpaces, vectorType_) {
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

  /* Ribbons
  if (isEnabled() && ribbonEnabled.get()) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {

      // Remap to center of each face (extrinsically)

      // Generate default tangent spaces if necessary
      if (!parent.hasFaceTangentSpaces()) {
        // TODO this could technically cause a problem if the user tries to register some other tangent spaces after
        // enabling the ribbon. This should really be an entirely internal implementation detail.
        parent.generateDefaultFaceTangentSpaces();
      }
      parent.ensureHaveFaceTangentSpaces();

      parent.ensureHaveVertexTangentSpaces();
      std::vector<glm::vec2> unitFaceVecs(parent.nFaces());
      for (size_t iF = 0; iF < parent.nFaces(); iF++) {
        std::vector<size_t>& face = parent.faces[iF];

        glm::vec3 faceBasisX = parent.faceTangentSpaces[iF][0];
        glm::vec3 faceBasisY = parent.faceTangentSpaces[iF][1];

        glm::vec2 sum{0.0, 0.0};
        for (size_t iV : face) {
          glm::vec2 vertVec = vectorField[iV];
          Complex angle = std::pow(Complex(vertVec.x, vertVec.y), 1.0 / nSym);
          vertVec = glm::vec2{angle.real(), angle.imag()};


          glm::vec3 vertexBasisX = parent.vertexTangentSpaces[iV][0];
          glm::vec3 vertexBasisY = parent.vertexTangentSpaces[iV][1];

          // Rotate in to the basis of the face
          glm::vec2 faceVec = rotateToTangentBasis(vertVec, vertexBasisX, vertexBasisY, faceBasisX, faceBasisY);
          angle = std::pow(Complex(faceVec.x, faceVec.y), nSym);
          faceVec = glm::vec2{angle.real(), angle.imag()};
          sum += faceVec;
        }
        unitFaceVecs[iF] = glm::normalize(sum);
      }

      // Warning: expensive... Creates noticeable UI lag
      ribbonArtist.reset(new RibbonArtist(parent, traceField(parent, unitFaceVecs, nSym, 2500)));
    }

    // Update transform matrix from parent
    ribbonArtist->draw();
  }
  */
}

void SurfaceVertexIntrinsicVectorQuantity::buildCustomUI() {
  buildVectorUI();

  /*
  if (ImGui::Checkbox("Draw ribbon", &ribbonEnabled.get())) setRibbonEnabled(isRibbonEnabled());
  if (ribbonEnabled.get() && ribbonArtist != nullptr) {
    ImGui::SameLine();
    ribbonArtist->buildParametersGUI();
  }
  */
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

std::vector<glm::vec2> oneFormToFaceTangentVectors(SurfaceMesh& mesh, const std::vector<double>& oneForm, std::vector<char>& canonicalOrientation) {

  mesh.vertexPositions.ensureHostBufferPopulated();
  mesh.faceAreas.ensureHostBufferPopulated();
  mesh.faceNormals.ensureHostBufferPopulated();
  mesh.defaultFaceTangentSpaces.ensureHostBufferPopulated();
  mesh.triangleEdgeInds.ensureHostBufferPopulated();
  mesh.triangleFaceInds.ensureHostBufferPopulated();

  std::vector<glm::vec2> mappedVectorField(mesh.nFaces());

  for (size_t iF = 0; iF < mesh.nFaces(); iF++) {

    std::array<float, 3> formValues;
    std::array<glm::vec3, 3> vecValues;
    for (size_t j = 0; j < 3; j++) {
      size_t vA = mesh.triangleFaceInds.data[3 * iF + j];
      size_t vB = mesh.triangleFaceInds.data[3 * iF + ((j + 1) % 3)];
      size_t iE = mesh.triangleEdgeInds.data[3 * iF + j];

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

    // Fill out data for the little arrows
    mappedVectorField[iF] = result;
  }

  return mappedVectorField;
}

} // namespace

SurfaceOneFormIntrinsicVectorQuantity::SurfaceOneFormIntrinsicVectorQuantity(std::string name,
                                                                             std::vector<double> oneForm_,
                                                                             std::vector<char> canonicalOrientation_,
                                                                             SurfaceMesh& mesh_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE),
      TangentVectorQuantity<SurfaceOneFormIntrinsicVectorQuantity>(*this, oneFormToFaceTangentVectors(mesh_, oneForm_, canonicalOrientation_),
                                                                   parent.faceCenters, parent.defaultFaceTangentSpaces,
                                                                   VectorType::STANDARD),
      oneForm(oneForm_),
      canonicalOrientation(canonicalOrientation_)


{
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

  /*
  if (isEnabled() && ribbonEnabled.get()) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {

      std::vector<glm::vec2> unitMappedField(parent.nFaces());
      for (size_t iF = 0; iF < parent.nFaces(); iF++) {
        unitMappedField[iF] = glm::normalize(mappedVectorField[iF]);
      }
      // Warning: expensive... Creates noticeable UI lag
      ribbonArtist.reset(new RibbonArtist(parent, traceField(parent, unitMappedField, 1, 2500)));
    }


    // Update transform matrix from parent
    ribbonArtist->draw();
  }
  */
}

void SurfaceOneFormIntrinsicVectorQuantity::buildCustomUI() {
  buildVectorUI();

  /*
  if (ImGui::Checkbox("Draw ribbon", &ribbonEnabled.get())) setRibbonEnabled(isRibbonEnabled());
  if (ribbonEnabled.get() && ribbonArtist != nullptr) {
    ImGui::SameLine();
    ribbonArtist->buildParametersGUI();
  }
  */
}

void SurfaceOneFormIntrinsicVectorQuantity::buildEdgeInfoGUI(size_t iE) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  ImGui::Text("%g", oneForm[iE]);

  ImGui::NextColumn();
}

std::string SurfaceOneFormIntrinsicVectorQuantity::niceName() { return name + " (1-form intrinsic vector)"; }

} // namespace polyscope
