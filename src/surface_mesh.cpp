#include "polyscope/surface_mesh.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/polyscope.h"

// Quantities
#include "polyscope/surface_scalar_quantity.h"
#include "polyscope/surface_vector_quantity.h"
#include "polyscope/surface_color_quantity.h"

#include "imgui.h"

using namespace geometrycentral;

namespace polyscope {

SurfaceMesh::SurfaceMesh(std::string name, Geometry<Euclidean>* geometry_)
    : Structure(name, StructureType::SurfaceMesh) {
  // Copy the mesh and save the transfer object
  mesh = geometry_->getMesh()->copy(transfer);
  geometry = geometry_->copyUsingTransfer(transfer);

  // surfaceColor = gl::RGB_SKYBLUE.toFloatArray();
  surfaceColor = getNextPaletteColor();

  prepare();
}

SurfaceMesh::~SurfaceMesh() {
  deleteProgram();

  // Delete quantities
  for (auto x : quantities) {
    delete x.second;
  }
}

void SurfaceMesh::deleteProgram() {
  if (program != nullptr) {
    delete program;
    program = nullptr;
  }
}

void SurfaceMesh::draw() {
  if (!enabled) {
    return;
  }

  if (program == nullptr) {
    prepare();
  }

  // Set uniforms
  glm::mat4 viewMat = view::getCameraViewMatrix();
  program->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  Vector3 eyePos = view::getCameraWorldPosition();
  program->setUniform("u_eye", eyePos);

  program->setUniform("u_lightCenter", state::center);
  program->setUniform("u_lightDist", 5 * state::lengthScale);
  program->setUniform("u_basecolor", surfaceColor);
  program->setUniform("u_edgeWidth", edgeWidth);

  program->draw();

  // Draw the quantities
  for (auto x : quantities) {
    x.second->draw();
  }
}

void SurfaceMesh::drawPick() {}

void SurfaceMesh::prepare() {
  // It not quantity is coloring the surface, draw with a default color
  if (activeSurfaceQuantity == nullptr) {
    program =
        new gl::GLProgram(&PLAIN_SURFACE_VERT_SHADER,
                          &PLAIN_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles);
  }
  // If some quantity is responsible for coloring the surface, prepare it
  else {
    program = activeSurfaceQuantity->createProgram();
  }

  // Populate draw buffers
  fillGeometryBuffers();
}

void SurfaceMesh::fillGeometryBuffers() {
  if (shadeStyle == ShadeStyle::SMOOTH) {
    fillGeometryBuffersSmooth();
  } else if (shadeStyle == ShadeStyle::FLAT) {
    fillGeometryBuffersFlat();
  }
}

void SurfaceMesh::fillGeometryBuffersSmooth() {
  std::vector<Vector3> positions;
  std::vector<Vector3> normals;
  std::vector<Vector3> bcoord;
  VertexData<Vector3> vertexNormals;
  geometry->getVertexNormals(vertexNormals);
  for (FacePtr f : mesh->faces()) {
    // Implicitly triangulate
    Vector3 p0, p1;
    Vector3 n0, n1;
    size_t iP = 0;
    for (VertexPtr v : f.adjacentVertices()) {
      Vector3 p2 = geometry->position(v);
      Vector3 n2 = vertexNormals[v];
      if (iP >= 2) {
        positions.push_back(p0);
        normals.push_back(n0);
        bcoord.push_back(Vector3{1.0, 0.0, 0.0});
        positions.push_back(p1);
        normals.push_back(n1);
        bcoord.push_back(Vector3{0.0, 1.0, 0.0});
        positions.push_back(p2);
        normals.push_back(n2);
        bcoord.push_back(Vector3{0.0, 0.0, 1.0});
      }
      p0 = p1;
      p1 = p2;
      n0 = n1;
      n1 = n2;
      iP++;
    }
  }

  // Store data in buffers
  program->setAttribute("a_position", positions);
  program->setAttribute("a_normal", normals);
  program->setAttribute("a_barycoord", bcoord);
}

void SurfaceMesh::fillGeometryBuffersFlat() {
  std::vector<Vector3> positions;
  std::vector<Vector3> normals;
  std::vector<Vector3> bcoord;

  FaceData<Vector3> faceNormals;
  geometry->getFaceNormals(faceNormals);
  for (FacePtr f : mesh->faces()) {
    // Implicitly triangulate
    Vector3 p0, p1;
    Vector3 n0, n1;
    size_t iP = 0;
    for (VertexPtr v : f.adjacentVertices()) {
      Vector3 p2 = geometry->position(v);
      Vector3 n2 = faceNormals[f];
      if (iP >= 2) {
        positions.push_back(p0);
        normals.push_back(n0);
        bcoord.push_back(Vector3{1.0, 0.0, 0.0});
        positions.push_back(p1);
        normals.push_back(n1);
        bcoord.push_back(Vector3{0.0, 1.0, 0.0});
        positions.push_back(p2);
        normals.push_back(n2);
        bcoord.push_back(Vector3{0.0, 0.0, 1.0});
      }
      p0 = p1;
      p1 = p2;
      n0 = n1;
      n1 = n2;
      iP++;
    }
  }

  // Store data in buffers
  program->setAttribute("a_position", positions);
  program->setAttribute("a_normal", normals);
  program->setAttribute("a_barycoord", bcoord);
}

void SurfaceMesh::drawSharedStructureUI() {}

void SurfaceMesh::drawUI() {
  ImGui::PushID(name.c_str());  // ensure there are no conflicts with
                                // identically-named labels

  if (ImGui::TreeNode(name.c_str())) {
    // enabled = true;

    ImGui::Checkbox("Enabled", &enabled);
    ImGui::SameLine();
    ImGui::ColorEdit3("Surface color", (float*)&surfaceColor,
                      ImGuiColorEditFlags_NoInputs);

    {  // Flat shading or smooth shading?
      ImGui::Checkbox("Smooth shade", &ui_smoothshade);
      if (ui_smoothshade && shadeStyle == ShadeStyle::FLAT) {
        shadeStyle = ShadeStyle::SMOOTH;
        deleteProgram();
      }
      if (!ui_smoothshade && shadeStyle == ShadeStyle::SMOOTH) {
        shadeStyle = ShadeStyle::FLAT;
        deleteProgram();
      }
      ImGui::SameLine();
    }

    {  // Edge width
      ImGui::Checkbox("Show edges", &showEdges);
      if (showEdges) {
        edgeWidth = 0.01;
      } else {
        edgeWidth = 0.0;
      }
    }

    // Draw the quantities
    for (auto x : quantities) {
      x.second->drawUI();
    }

    ImGui::TreePop();
  } else {
    // enabled = false;
  }
  ImGui::PopID();
}

double SurfaceMesh::lengthScale() {
  // Measure length scale as twice the radius from the center of the bounding
  // box
  auto bound = boundingBox();
  Vector3 center = 0.5 * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (VertexPtr v : mesh->vertices()) {
    lengthScale = std::max(
        lengthScale, geometrycentral::norm2(geometry->position(v) - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
SurfaceMesh::boundingBox() {
  Vector3 min = Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();
  Vector3 max = -Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();

  for (VertexPtr v : mesh->vertices()) {
    min = geometrycentral::componentwiseMin(min, geometry->position(v));
    max = geometrycentral::componentwiseMax(max, geometry->position(v));
  }

  return std::make_tuple(min, max);
}
  
void SurfaceMesh::updateGeometryPositions(Geometry<Euclidean>* newGeometry) {

  VertexData<Vector3> newPositions;
  newGeometry->getVertexPositions(newPositions);

  VertexData<Vector3> myNewPositions = transfer.transfer(newPositions);
  for(VertexPtr v : mesh->vertices()) { 
    geometry->position(v) = myNewPositions[v];
  }

  // Rebuild any necessary quantities
  deleteProgram();
  prepare();
}

SurfaceQuantity::SurfaceQuantity(std::string name_, SurfaceMesh* mesh_)
    : name(name_), parent(mesh_) {}
SurfaceQuantityThatDrawsFaces::SurfaceQuantityThatDrawsFaces(std::string name_,
                                                             SurfaceMesh* mesh_)
    : SurfaceQuantity(name_, mesh_) {}
SurfaceQuantity::~SurfaceQuantity() {}

void SurfaceMesh::addQuantity(std::string name, VertexData<double>& value,
                              DataType type) {
  // Delete old if in use
  if (quantities.find(name) != quantities.end()) {
    removeQuantity(name);
  }

  SurfaceScalarQuantity* q =
      new SurfaceScalarVertexQuantity(name, value, this, type);
  quantities[name] = q;
}

void SurfaceMesh::addQuantity(std::string name, FaceData<double>& value,
                              DataType type) {
  // Delete old if in use
  if (quantities.find(name) != quantities.end()) {
    removeQuantity(name);
  }

  SurfaceScalarQuantity* q =
      new SurfaceScalarFaceQuantity(name, value, this, type);
  quantities[name] = q;
}

void SurfaceMesh::addQuantity(std::string name, EdgeData<double>& value,
                              DataType type) {
  // Delete old if in use
  if (quantities.find(name) != quantities.end()) {
    removeQuantity(name);
  }

  SurfaceScalarQuantity* q =
      new SurfaceScalarEdgeQuantity(name, value, this, type);
  quantities[name] = q;
}

void SurfaceMesh::addQuantity(std::string name, HalfedgeData<double>& value,
                              DataType type) {
  // Delete old if in use
  if (quantities.find(name) != quantities.end()) {
    removeQuantity(name);
  }

  SurfaceScalarQuantity* q =
      new SurfaceScalarHalfedgeQuantity(name, value, this, type);
  quantities[name] = q;
}
  
void SurfaceMesh::addColorQuantity(std::string name, VertexData<Vector3>& value) {

  if (quantities.find(name) != quantities.end()) {
    removeQuantity(name);
  }

  SurfaceColorQuantity* q =
      new SurfaceColorVertexQuantity(name, value, this);
  quantities[name] = q;

}

void SurfaceMesh::addVectorQuantity(std::string name,
                                    VertexData<Vector3>& value,
                                    VectorType vectorType) {
  // Delete old if in use
  if (quantities.find(name) != quantities.end()) {
    removeQuantity(name);
  }

  SurfaceVectorQuantity* q =
      new SurfaceVectorQuantity(name, value, this, vectorType);
  quantities[name] = q;
}

void SurfaceMesh::addVectorQuantity(std::string name, FaceData<Vector3>& value,
                                    VectorType vectorType) {
  // Delete old if in use
  if (quantities.find(name) != quantities.end()) {
    removeQuantity(name);
  }

  SurfaceVectorQuantity* q =
      new SurfaceVectorQuantity(name, value, this, vectorType);
  quantities[name] = q;
}

void SurfaceMesh::removeQuantity(std::string name) {
  if (quantities.find(name) != quantities.end()) {
    return;
  }

  SurfaceQuantity* q = quantities[name];
  quantities.erase(name);
  if (activeSurfaceQuantity == q) {
    clearActiveSurfaceQuantity();
  }
  delete q;
}

void SurfaceMesh::setActiveSurfaceQuantity(SurfaceQuantityThatDrawsFaces* q) {
  clearActiveSurfaceQuantity();
  activeSurfaceQuantity = q;
}

void SurfaceMesh::clearActiveSurfaceQuantity() {
  deleteProgram();
  if (activeSurfaceQuantity != nullptr) {
    activeSurfaceQuantity->enabled = false;
    activeSurfaceQuantity = nullptr;
  }
}

}  // namespace polyscope