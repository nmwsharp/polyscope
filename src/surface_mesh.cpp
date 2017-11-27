#include "polyscope/surface_mesh.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using namespace geometrycentral;

namespace polyscope {

SurfaceMesh::SurfaceMesh(std::string name, Geometry<Euclidean>* geometry_)
    : Structure(name, StructureType::SurfaceMesh) {
  // Copy the mesh and save the transfer object
  mesh = geometry_->getMesh()->copy(transfer);
  geometry = geometry_->copyUsingTransfer(transfer);

  surfaceColor = gl::RGB_SKYBLUE.toFloatArray();

  prepare();
}

void SurfaceMesh::draw() {
  if (!enabled) {
    return;
  }

  // Set uniforms
  glm::mat4 viewMat = view::getViewMatrix();
  program->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  Vector3 eyePos = view::getCameraWorldPosition();
  program->setUniform("u_eye", eyePos);

  program->setUniform("u_lightCenter", state::center);
  program->setUniform("u_lightDist", 5 * state::lengthScale);
  program->setUniform("u_color", surfaceColor);

  program->draw();
}

void SurfaceMesh::drawPick() {}

void SurfaceMesh::prepare() {
  // Create the GL program
  program =
      new gl::GLProgram(&PLAIN_SURFACE_VERT_SHADER, &PLAIN_SURFACE_FRAG_SHADER,
                        gl::DrawMode::Triangles);

  // Populate draw buffers
  std::vector<Vector3> positions;
  std::vector<Vector3> normals;
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
        positions.push_back(p1);
        normals.push_back(n1);
        positions.push_back(p2);
        normals.push_back(n2);
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
}

void SurfaceMesh::teardown() {
  if (program != nullptr) {
    delete program;
  }
}

void SurfaceMesh::drawUI() {
  ImGui::PushID(name.c_str());  // ensure there are no conflicts with
                                // identically-named labels

  ImGui::TextUnformatted(name.c_str());
  ImGui::Checkbox("Enabled", &enabled);
  ImGui::ColorEdit3("Surface color", (float*)&surfaceColor,
                    ImGuiColorEditFlags_NoInputs);

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

}  // namespace polyscope