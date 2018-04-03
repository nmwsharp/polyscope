#include "polyscope/surface_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/vector_shaders.h"
#include "polyscope/polyscope.h"
#include "polyscope/trace_vector_field.h"

#include "imgui.h"

#include "Eigen/Dense"

#include <complex>
#include <fstream>
#include <iostream>

using std::cout;
using std::endl;

namespace polyscope {

SurfaceVectorQuantity::SurfaceVectorQuantity(std::string name, SurfaceMesh* mesh_, MeshElement definedOn_,
                                             VectorType vectorType_)

    : SurfaceQuantity(name, mesh_), vectorType(vectorType_), definedOn(definedOn_) {

  // Don't forget to call finishConstructing() in children classes!
}

SurfaceVectorQuantity::~SurfaceVectorQuantity() {
  safeDelete(program);
  safeDelete(ribbonArtist);
}

void SurfaceVectorQuantity::finishConstructing() {

  // Create a mapper (default mapper is identity)
  if (vectorType == VectorType::AMBIENT) {
    mapper.setMinMax(vectors);
  } else {
    mapper = AffineRemapper<Vector3>(vectors, DataType::MAGNITUDE);
  }

  // Default viz settings
  if (vectorType != VectorType::AMBIENT) {
    lengthMult = .02;
  } else {
    lengthMult = 1.0;
  }
  radiusMult = .0005;
  vectorColor = parent->colorManager.getNextSubColor(name);
}

void SurfaceVectorQuantity::draw() {
  if (!enabled || ribbonEnabled) return;

  if (program == nullptr) prepare();

  // Set uniforms
  glm::mat4 viewMat = parent->getModelView();
  program->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  Vector3 eyePos = view::getCameraWorldPosition();
  program->setUniform("u_eye", eyePos);

  program->setUniform("u_lightCenter", state::center);
  program->setUniform("u_lightDist", 5 * state::lengthScale);
  program->setUniform("u_radius", radiusMult * state::lengthScale);
  program->setUniform("u_color", vectorColor);

  if (vectorType == VectorType::AMBIENT) {
    program->setUniform("u_lengthMult", 1.0);
  } else {
    program->setUniform("u_lengthMult", lengthMult * state::lengthScale);
  }

  program->draw();
}

void SurfaceVectorQuantity::prepare() {
  program = new gl::GLProgram(&PASSTHRU_VECTOR_VERT_SHADER, &VECTOR_GEOM_SHADER, &SHINY_VECTOR_FRAG_SHADER,
                              gl::DrawMode::Points);

  // Fill buffers
  std::vector<Vector3> mappedVectors;
  for (Vector3 v : vectors) {
    mappedVectors.push_back(mapper.map(v));
  }

  program->setAttribute("a_vector", mappedVectors);
  program->setAttribute("a_position", vectorRoots);
}

void SurfaceVectorQuantity::drawUI() {


  if (ImGui::TreeNode((name + " (" + getMeshElementTypeName(definedOn) + " vector)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);
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

    ImGui::TreePop();
  }
}

void SurfaceVectorQuantity::drawSubUI() {}

void SurfaceVectorQuantity::writeToFile(std::string filename) {

  if (filename == "") {
    filename = promptForFilename();
    if (filename == "") {
      return;
    }
  }

  cout << "Writing surface vector quantity " << name << " to file " << filename << endl;

  std::ofstream outFile(filename);
  outFile << "#Vectors written by polyscope from Surface Vector Quantity " << name << endl;
  outFile << "#displayradius " << (radiusMult * state::lengthScale) << endl;
  outFile << "#displaylength " << (lengthMult * state::lengthScale) << endl;

  for (size_t i = 0; i < vectors.size(); i++) {
    if (norm(vectors[i]) > 0) {
      outFile << vectorRoots[i] << " " << vectors[i] << endl;
    }
  }

  outFile.close();
}


// ========================================================
// ==========           Vertex Vector            ==========
// ========================================================

SurfaceVertexVectorQuantity::SurfaceVertexVectorQuantity(std::string name, VertexData<Vector3>& vectors_,
                                                         SurfaceMesh* mesh_, VectorType vectorType_)

    : SurfaceVectorQuantity(name, mesh_, MeshElement::VERTEX, vectorType_) {

  vectorField = parent->transfer.transfer(vectors_);
  for (VertexPtr v : parent->mesh->vertices()) {
    vectorRoots.push_back(parent->geometry->position(v));
    vectors.push_back(vectorField[v]);
  }

  finishConstructing();
}

void SurfaceVertexVectorQuantity::buildInfoGUI(VertexPtr v) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectorField[v];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", norm(vectorField[v]));
  ImGui::NextColumn();
}


// ========================================================
// ==========            Face Vector             ==========
// ========================================================

SurfaceFaceVectorQuantity::SurfaceFaceVectorQuantity(std::string name, FaceData<Vector3>& vectors_, SurfaceMesh* mesh_,
                                                     VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE, vectorType_) {

  // Copy the vectors
  vectorField = parent->transfer.transfer(vectors_);
  for (FacePtr f : parent->mesh->faces()) {
    vectorRoots.push_back(parent->geometry->barycenter(f));
    vectors.push_back(vectorField[f]);
  }

  finishConstructing();
}

void SurfaceFaceVectorQuantity::buildInfoGUI(FacePtr f) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectorField[f];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", norm(vectorField[f]));
  ImGui::NextColumn();
}

// ========================================================
// ==========        Intrinsic Face Vector       ==========
// ========================================================


SurfaceFaceIntrinsicVectorQuantity::SurfaceFaceIntrinsicVectorQuantity(std::string name, FaceData<Complex>& vectors_,
                                                                       SurfaceMesh* mesh_, int nSym_,
                                                                       VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE, vectorType_), nSym(nSym_) {

  GeometryCache<Euclidean>& gc = parent->geometry->cache;
  gc.requireFaceBases();

  double rotAngle = 2.0 * PI / nSym;
  Complex rot = std::exp(IM_I * rotAngle);

  // Copy the vectors
  vectorField = parent->transfer.transfer(vectors_);
  for (FacePtr f : parent->mesh->faces()) {

    Complex angle = std::pow(vectorField[f], 1.0 / nSym);

    for (int iRot = 0; iRot < nSym; iRot++) {
      vectorRoots.push_back(parent->geometry->barycenter(f));

      Vector3 v = gc.faceBases[f][0] * angle.real() + gc.faceBases[f][1] * angle.imag();
      vectors.push_back(v);

      angle *= rot;
    }
  }

  finishConstructing();
}

void SurfaceFaceIntrinsicVectorQuantity::buildInfoGUI(FacePtr f) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectorField[f];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", std::abs(vectorField[f]));
  ImGui::NextColumn();
}

void SurfaceFaceIntrinsicVectorQuantity::draw() {
  SurfaceVectorQuantity::draw();

  if (ribbonEnabled) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {

      // Warning: expensive... Creates noticeable UI lag
      ribbonArtist = new RibbonArtist(traceField(parent->geometry, vectorField, nSym, 2500));
    }


    if (enabled) {

      // Update transform matrix from parent
      ribbonArtist->objectTransform = parent->objectTransform;

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


// ========================================================
// ==========       Intrinsic Vertex Vector      ==========
// ========================================================


SurfaceVertexIntrinsicVectorQuantity::SurfaceVertexIntrinsicVectorQuantity(std::string name,
                                                                           VertexData<Complex>& vectors_,
                                                                           SurfaceMesh* mesh_, int nSym_,
                                                                           VectorType vectorType_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::VERTEX, vectorType_), nSym(nSym_) {

  GeometryCache<Euclidean>& gc = parent->geometry->cache;
  gc.requireVertexBases();

  double rotAngle = 2.0 * PI / nSym;
  Complex rot = std::exp(IM_I * rotAngle);

  // Copy the vectors
  vectorField = parent->transfer.transfer(vectors_);
  for (VertexPtr v : parent->mesh->vertices()) {

    Complex angle = std::pow(vectorField[v], 1.0 / nSym);

    for (int iRot = 0; iRot < nSym; iRot++) {
      vectorRoots.push_back(parent->geometry->position(v));

      Vector3 vec = gc.vertexBases[v][0] * angle.real() + gc.vertexBases[v][1] * angle.imag();
      vectors.push_back(vec);

      angle *= rot;
    }
  }

  finishConstructing();
}

void SurfaceVertexIntrinsicVectorQuantity::buildInfoGUI(VertexPtr v) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectorField[v];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", std::abs(vectorField[v]));
  ImGui::NextColumn();
}

void SurfaceVertexIntrinsicVectorQuantity::draw() {
  SurfaceVectorQuantity::draw();

  if (ribbonEnabled) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {

      // Remap to center of each face
      GeometryCache<Euclidean>& gc = parent->geometry->cache;
      gc.requireVertexFaceTransportCoefs();
      FaceData<Complex> unitFaceVecs(parent->mesh);
      for (FacePtr f : parent->mesh->faces()) {

        Complex sum{0.0, 0.0};
        for (HalfedgePtr he : f.adjacentHalfedges()) {
          Complex valInFace = std::pow(gc.vertexFaceTransportCoefs[he], nSym) * vectorField[he.vertex()];
          sum += valInFace;
        }
        unitFaceVecs[f] = unit(sum);
      }

      // Warning: expensive... Creates noticeable UI lag
      ribbonArtist = new RibbonArtist(traceField(parent->geometry, unitFaceVecs, nSym, 2500));
    }


    if (enabled) {

      // Update transform matrix from parent
      ribbonArtist->objectTransform = parent->objectTransform;

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

// ========================================================
// ==========        Intrinsic One Form          ==========
// ========================================================


SurfaceOneFormIntrinsicVectorQuantity::SurfaceOneFormIntrinsicVectorQuantity(std::string name,
                                                                             EdgeData<double>& oneForm_,
                                                                             SurfaceMesh* mesh_)
    : SurfaceVectorQuantity(name, mesh_, MeshElement::FACE, VectorType::STANDARD) {

  GeometryCache<Euclidean>& gc = parent->geometry->cache;
  gc.requireFaceBases();
  gc.requireFaceAreas();
  gc.requireHalfedgeVectors();
  gc.requireFaceNormals();

  // Copy the vectors
  oneForm = parent->transfer.transfer(oneForm_);
  mappedVectorField = FaceData<Complex>(parent->mesh);
  for (FacePtr f : parent->mesh->faces()) {

    // Whitney interpolation at center
    std::array<double, 3> formValues;
    std::array<Vector3, 3> vecValues;
    int i = 0;
    for (HalfedgePtr he : f.adjacentHalfedges()) {
      double signVal = (he == he.edge().halfedge()) ? 1.0 : -1.0;
      formValues[i] = oneForm[he.edge()] * signVal;
      vecValues[i] = cross(gc.halfedgeVectors[he], gc.faceNormals[f]);
      i++;
    }
    Vector3 result = Vector3::zero();
    for (int j = 0; j < 3; j++) {
      result += (formValues[(j + 1) % 3] - formValues[(j + 2) % 3]) * vecValues[j];
    }
    result /= 6 * gc.faceAreas[f];

    Complex approxVec{dot(result, gc.faceBases[f][0]), dot(result, gc.faceBases[f][1])};
    mappedVectorField[f] = approxVec;


    // Fill out data for the little arrows
    vectorRoots.push_back(parent->geometry->barycenter(f));
    Vector3 v = gc.faceBases[f][0] * approxVec.real() + gc.faceBases[f][1] * approxVec.imag();
    vectors.push_back(v);
  }

  finishConstructing();
}

void SurfaceOneFormIntrinsicVectorQuantity::buildInfoGUI(EdgePtr e) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  ImGui::Text("%g", oneForm[e]);

  ImGui::NextColumn();
}

void SurfaceOneFormIntrinsicVectorQuantity::buildInfoGUI(FacePtr f) {
  ImGui::TextUnformatted((name + "(remapped)").c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << mappedVectorField[f];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", std::abs(mappedVectorField[f]));
  ImGui::NextColumn();
}

void SurfaceOneFormIntrinsicVectorQuantity::draw() {
  SurfaceVectorQuantity::draw();

  if (ribbonEnabled) {

    // Make sure we have a ribbon artist
    if (ribbonArtist == nullptr) {

      // Warning: expensive... Creates noticeable UI lag
      FaceData<Complex> unitMappedField(parent->mesh);
      for (FacePtr f : parent->mesh->faces()) {
        unitMappedField[f] = mappedVectorField[f] / std::abs(mappedVectorField[f]);
      }
      ribbonArtist = new RibbonArtist(traceField(parent->geometry, unitMappedField, 1, 5000));
    }


    if (enabled) {
      // Update transform matrix from parent
      ribbonArtist->objectTransform = parent->objectTransform;

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

} // namespace polyscope
