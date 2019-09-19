// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/vector_shaders.h"
#include "polyscope/polyscope.h"
#include "polyscope/trace_vector_field.h"

#include "imgui.h"

#include <complex>
#include <fstream>
#include <iostream>

using std::cout;
using std::endl;

namespace polyscope {

SurfaceVectorQuantity::SurfaceVectorQuantity(std::string name, SurfaceMesh& mesh_, MeshElement definedOn_,
                                             VectorType vectorType_)
    : SurfaceMeshQuantity(name, mesh_), vectorType(vectorType_), definedOn(definedOn_) {}

void SurfaceVectorQuantity::prepareVectorMapper() {

  // Create a mapper (default mapper is identity)
  if (vectorType == VectorType::AMBIENT) {
    mapper.setMinMax(vectors);
  } else {
    mapper = AffineRemapper<glm::vec3>(vectors, DataType::MAGNITUDE);
  }

  // Default viz settings
  if (vectorType != VectorType::AMBIENT) {
    lengthMult = .02;
  } else {
    lengthMult = 1.0;
  }
  radiusMult = .0005;
  vectorColor = getNextUniqueColor();
}

void SurfaceVectorQuantity::draw() {
  if (!enabled) return;

  if (program == nullptr) prepareProgram();

  // Set uniforms
  parent.setTransformUniforms(*program);

  program->setUniform("u_radius", radiusMult * state::lengthScale);
  program->setUniform("u_color", vectorColor);

  if (vectorType == VectorType::AMBIENT) {
    program->setUniform("u_lengthMult", 1.0);
  } else {
    program->setUniform("u_lengthMult", lengthMult * state::lengthScale);
  }

  program->draw();
}

void SurfaceVectorQuantity::prepareProgram() {

  program.reset(new gl::GLProgram(&gl::PASSTHRU_VECTOR_VERT_SHADER, &gl::VECTOR_GEOM_SHADER,
                                  &gl::SHINY_VECTOR_FRAG_SHADER, gl::DrawMode::Points));

  // Fill buffers
  std::vector<glm::vec3> mappedVectors;
  for (glm::vec3& v : vectors) {
    mappedVectors.push_back(mapper.map(v));
  }

  program->setAttribute("a_vector", mappedVectors);
  program->setAttribute("a_position", vectorRoots);

  setMaterialForProgram(*program, "wax");
}

void SurfaceVectorQuantity::buildCustomUI() {
  ImGui::SameLine();
  ImGui::ColorEdit3("Color", (float*)&vectorColor, ImGuiColorEditFlags_NoInputs);
  ImGui::SameLine();


  // === Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {
    if (ImGui::MenuItem("Write to file")) writeToFile();
    ImGui::EndPopup();
  }


  // Only get to set length for non-ambient vectors
  if (vectorType != VectorType::AMBIENT) {
    ImGui::SliderFloat("Length", &lengthMult, 0.0, .1, "%.5f", 3.);
  }

  ImGui::SliderFloat("Radius", &radiusMult, 0.0, .1, "%.5f", 3.);

  { // Draw max and min magnitude
    ImGui::TextUnformatted(mapper.printBounds().c_str());
  }

  drawSubUI();
}

void SurfaceVectorQuantity::drawSubUI() {}

void SurfaceVectorQuantity::writeToFile(std::string filename) {

  if (filename == "") {
    filename = promptForFilename();
    if (filename == "") {
      return;
    }
  }

  if (options::verbosity > 0) {
    cout << "Writing surface vector quantity " << name << " to file " << filename << endl;
  }

  std::ofstream outFile(filename);
  outFile << "#Vectors written by polyscope from Surface Vector Quantity " << name << endl;
  outFile << "#displayradius " << (radiusMult * state::lengthScale) << endl;
  outFile << "#displaylength " << (lengthMult * state::lengthScale) << endl;

  for (size_t i = 0; i < vectors.size(); i++) {
    if (glm::length(vectors[i]) > 0) {
      outFile << vectorRoots[i] << " " << vectors[i] << endl;
    }
  }

  outFile.close();
}

// ========================================================
// ==========           Vertex Vector            ==========
// ========================================================

SurfaceVertexVectorQuantity::SurfaceVertexVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                         SurfaceMesh& mesh_, VectorType vectorType_)

    : SurfaceVectorQuantity(name, mesh_, MeshElement::VERTEX, vectorType_), vectorField(vectors_) {

  size_t i = 0;
  vectorRoots = parent.vertices;
  vectors = vectorField;

  prepareVectorMapper();
}

void SurfaceVertexVectorQuantity::buildVertexInfoGUI(size_t iV) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectorField[iV];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vectorField[iV]));
  ImGui::NextColumn();
}

std::string SurfaceVertexVectorQuantity::niceName() { return name + " (vertex vector)"; }

// ========================================================
// ==========            Face Vector             ==========
// ========================================================

SurfaceFaceVectorQuantity::SurfaceFaceVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                     SurfaceMesh& mesh_, VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE, vectorType_), vectorField(vectors_) {

  // Copy the vectors
  vectors = vectorField;
  vectorRoots.resize(parent.nFaces());
  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();
    glm::vec3 faceCenter = parent.faceCenter(iF);
    vectorRoots[iF] = faceCenter;
  }

  prepareVectorMapper();
}

void SurfaceFaceVectorQuantity::buildFaceInfoGUI(size_t iF) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectorField[iF];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vectorField[iF]));
  ImGui::NextColumn();
}

std::string SurfaceFaceVectorQuantity::niceName() { return name + " (face vector)"; }

// ========================================================
// ==========        Intrinsic Face Vector       ==========
// ========================================================


SurfaceFaceIntrinsicVectorQuantity::SurfaceFaceIntrinsicVectorQuantity(std::string name,
                                                                       std::vector<glm::vec2> vectors_,
                                                                       SurfaceMesh& mesh_, int nSym_,
                                                                       VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE, vectorType_), nSym(nSym_), vectorField(vectors_) {

  parent.ensureHaveFaceTangentSpaces();

  double rotAngle = 2.0 * PI / nSym;
  Complex rot = std::exp(Complex(0, 1) * rotAngle);

  // Copy the vectors
  for (size_t iF = 0; iF < parent.nFaces(); iF++) {

    glm::vec3 normal = parent.faceNormals[iF];
    glm::vec3 basisX = parent.faceTangentSpaces[iF][0];
    glm::vec3 basisY = parent.faceTangentSpaces[iF][1];

    glm::vec2 vec = vectorField[iF];
    Complex angle = std::pow(Complex(vec.x, vec.y), 1.0 / nSym);

    // Face center
    auto& face = parent.faces[iF];
    size_t D = face.size();
    glm::vec3 faceCenter = parent.faceCenter(iF);

    for (int iRot = 0; iRot < nSym; iRot++) {
      vectorRoots.push_back(faceCenter);

      glm::vec3 vec = basisX * (float)angle.real() + basisY * (float)angle.imag();
      vectors.push_back(vec);

      angle *= rot;
    }
  }

  prepareVectorMapper();
}

void SurfaceFaceIntrinsicVectorQuantity::buildFaceInfoGUI(size_t iF) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << "<" << vectorField[iF].x << "," << vectorField[iF].y << ">";
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vectorField[iF]));
  ImGui::NextColumn();
}

void SurfaceFaceIntrinsicVectorQuantity::draw() {
  SurfaceVectorQuantity::draw();

  if (ribbonEnabled) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {
      // Warning: expensive... Creates noticeable UI lag
      ribbonArtist.reset(new RibbonArtist(parent, traceField(parent, vectorField, nSym, 2500)));
    }

    if (enabled) {
      // Update transform matrix from parent
      ribbonArtist->objectTransform = parent.objectTransform;
      ribbonArtist->draw();
    }
  }
}

void SurfaceFaceIntrinsicVectorQuantity::drawSubUI() {

  ImGui::Checkbox("Draw ribbon", &ribbonEnabled);
  if (ribbonEnabled && ribbonArtist != nullptr) {
    ribbonArtist->buildParametersGUI();
  }
}

std::string SurfaceFaceIntrinsicVectorQuantity::niceName() { return name + " (face intrinsic vector)"; }

// ========================================================
// ==========       Intrinsic Vertex Vector      ==========
// ========================================================


SurfaceVertexIntrinsicVectorQuantity::SurfaceVertexIntrinsicVectorQuantity(std::string name,
                                                                           std::vector<glm::vec2> vectors_,
                                                                           SurfaceMesh& mesh_, int nSym_,
                                                                           VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::VERTEX, vectorType_), nSym(nSym_), vectorField(vectors_) {

  parent.ensureHaveVertexTangentSpaces();

  double rotAngle = 2.0 * PI / nSym;
  Complex rot = std::exp(Complex(0, 1) * rotAngle);

  // Copy the vectors
  for (size_t iV = 0; iV < parent.nVertices(); iV++) {

    glm::vec3 normal = parent.vertexNormals[iV];
    glm::vec3 basisX = parent.vertexTangentSpaces[iV][0];
    glm::vec3 basisY = parent.vertexTangentSpaces[iV][1];

    glm::vec2 vec = vectorField[iV];
    Complex angle = std::pow(Complex(vec.x, vec.y), 1.0 / nSym);

    for (int iRot = 0; iRot < nSym; iRot++) {
      vectorRoots.push_back(parent.vertices[iV]);

      glm::vec3 vec = basisX * (float)angle.real() + basisY * (float)angle.imag();
      vectors.push_back(vec);

      angle *= rot;
    }
  }

  prepareVectorMapper();
}

void SurfaceVertexIntrinsicVectorQuantity::buildVertexInfoGUI(size_t iV) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << "<" << vectorField[iV].x << "," << vectorField[iV].y << ">";
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vectorField[iV]));
  ImGui::NextColumn();
}

void SurfaceVertexIntrinsicVectorQuantity::draw() {
  SurfaceVectorQuantity::draw();

  if (ribbonEnabled) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {

      // Remap to center of each face (extrinsically)
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

          glm::vec3 vertexBasisX = parent.vertexTangentSpaces[iV][0];
          glm::vec3 vertexBasisY = parent.vertexTangentSpaces[iV][1];

          // Rotate in to the basis of the face
          glm::vec2 faceVec = rotateToTangentBasis(vertVec, vertexBasisX, vertexBasisY, faceBasisX, faceBasisY);
          sum += faceVec;
        }
        unitFaceVecs[iF] = glm::normalize(sum);
      }

      // Warning: expensive... Creates noticeable UI lag
      ribbonArtist.reset(new RibbonArtist(parent, traceField(parent, unitFaceVecs, nSym, 2500)));
    }


    if (enabled) {
      // Update transform matrix from parent
      ribbonArtist->objectTransform = parent.objectTransform;
      ribbonArtist->draw();
    }
  }
}

void SurfaceVertexIntrinsicVectorQuantity::drawSubUI() {

  ImGui::Checkbox("Draw ribbon", &ribbonEnabled);
  if (ribbonEnabled && ribbonArtist != nullptr) {
    ribbonArtist->buildParametersGUI();
  }
}

std::string SurfaceVertexIntrinsicVectorQuantity::niceName() { return name + " (vertex intrinsic vector)"; }

// ========================================================
// ==========        Intrinsic One Form          ==========
// ========================================================


SurfaceOneFormIntrinsicVectorQuantity::SurfaceOneFormIntrinsicVectorQuantity(std::string name,
                                                                             std::vector<double> oneForm_,
                                                                             std::vector<char> canonicalOrientation_,
                                                                             SurfaceMesh& mesh_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE, VectorType::STANDARD), oneForm(oneForm_) {

  vectorRoots = std::vector<glm::vec3>(parent.nFaces(), glm::vec3{0., 0., 0.});
  vectors = std::vector<glm::vec3>(parent.nFaces(), glm::vec3{0., 0., 0.});
  mappedVectorField = std::vector<glm::vec2>(parent.nFaces(), glm::vec3{0., 0., 0.});

  // Remap to faces
  parent.ensureHaveFaceTangentSpaces();
  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();

    // sorry, need triangles
    if (D != 3) {
      warning("tried to visualize 1-form with non-triangular face");
      continue;
    }

    // find the face center
    glm::vec3 faceCenter = parent.faceCenter(iF);
    vectorRoots[iF] = faceCenter;

    std::array<float, 3> formValues;
    std::array<glm::vec3, 3> vecValues;
    for (size_t j = 0; j < D; j++) {
      size_t vA = face[j];
      size_t vB = face[(j + 1) % D];
      size_t iE = parent.edgeIndices[iF][j];

      bool isCanonicalOriented;
      if (parent.vertexPerm.size() > 0) {
        isCanonicalOriented = ((parent.vertexPerm[vB] > parent.vertexPerm[vA]) != canonicalOrientation_[iE]);
      } else {
        isCanonicalOriented = ((vB > vA) != canonicalOrientation_[iE]);
      }
      double orientationSign = isCanonicalOriented ? 1. : -1.;

      formValues[j] = orientationSign * oneForm[iE];

      glm::vec3 heVec = parent.vertices[vB] - parent.vertices[vA];
      vecValues[j] = glm::cross(heVec, parent.faceNormals[iF]);
    }


    // Whitney interpolation at center
    glm::vec3 result{0., 0., 0.};
    for (int j = 0; j < 3; j++) {
      result += (formValues[(j + 1) % 3] - formValues[(j + 2) % 3]) * vecValues[j];
    }
    result /= static_cast<float>(6. * parent.faceAreas[iF]);

    glm::vec2 approxVec{glm::dot(result, parent.faceTangentSpaces[iF][0]),
                        glm::dot(result, parent.faceTangentSpaces[iF][1])};
    mappedVectorField[iF] = approxVec;

    // Fill out data for the little arrows
    vectors[iF] = result;
  }

  prepareVectorMapper();
}

void SurfaceOneFormIntrinsicVectorQuantity::buildEdgeInfoGUI(size_t iE) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  ImGui::Text("%g", oneForm[iE]);

  ImGui::NextColumn();
}

void SurfaceOneFormIntrinsicVectorQuantity::buildFaceInfoGUI(size_t iF) {
  ImGui::TextUnformatted((name + "(remapped)").c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << "<" << mappedVectorField[iF].x << "," << mappedVectorField[iF].y << ">";
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(mappedVectorField[iF]));
  ImGui::NextColumn();
}

void SurfaceOneFormIntrinsicVectorQuantity::draw() {
  SurfaceVectorQuantity::draw();

  if (ribbonEnabled) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {

      std::vector<glm::vec2> unitMappedField(parent.nFaces());
      for (size_t iF = 0; iF < parent.nFaces(); iF++) {
        unitMappedField[iF] = glm::normalize(mappedVectorField[iF]);
      }
      // Warning: expensive... Creates noticeable UI lag
      ribbonArtist.reset(new RibbonArtist(parent, traceField(parent, unitMappedField, 1, 2500)));
    }


    if (enabled) {
      // Update transform matrix from parent
      ribbonArtist->objectTransform = parent.objectTransform;
      ribbonArtist->draw();
    }
  }
}

void SurfaceOneFormIntrinsicVectorQuantity::drawSubUI() {

  ImGui::Checkbox("Draw ribbon", &ribbonEnabled);
  if (ribbonEnabled && ribbonArtist != nullptr) {
    ribbonArtist->buildParametersGUI();
  }
}

std::string SurfaceOneFormIntrinsicVectorQuantity::niceName() { return name + " (1-form intrinsic vector)"; }

} // namespace polyscope
