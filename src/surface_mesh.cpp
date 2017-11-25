#include "polyscope/surface_mesh.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using namespace geometrycentral;

namespace polyscope {

SurfaceMesh::SurfaceMesh(std::string name, Geometry<Euclidean>* geometry_)
    : Structure(name) {

  // Copy the mesh and save the transfer object
  mesh = geometry_->getMesh()->copy(transfer); 
  geometry = geometry_->copyUsingTransfer(transfer);

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

  // Vector3 lightPos = view::getLightWorldPosition();center
  program->setUniform("u_lightC", state::center);
  program->setUniform("u_lightD", 2*state::lengthScale);
  
  program->setUniform("u_camZ", view::cameraDirection);
  program->setUniform("u_camUp", view::upDirection);
  Vector3 leftHand = unit(cross(view::cameraDirection, view::upDirection));
  program->setUniform("u_camRight", -leftHand); 
  
  program->setUniform("u_pointRadius", 0.001 * state::lengthScale);

  program->draw();
}

void SurfaceMesh::drawPick() {}

void SurfaceMesh::prepare() {

  // // Create the GL program
  // program = new gl::GLProgram(&PASSTHRU_SPHERE_VERT_SHADER, &SPHERE_GEOM_BILLBOARD_SHADER,
  //                             &SHINY_SPHERE_FRAG_SHADER, gl::DrawMode::Points);

  // // Constant color
  // std::vector<Vector3> colorData(points.size());
  // std::fill(colorData.begin(), colorData.end(), gl::RGB_ORANGE);

  // // Store data in buffers
  // program->setAttribute("a_position", points);
  // program->setAttribute("a_color", colorData);
}

void SurfaceMesh::teardown() {
  if (program != nullptr) {
    delete program;
  }
}

void SurfaceMesh::drawUI() {

  ImGui::TextUnformatted(name.c_str());
  ImGui::Checkbox("Enabled", &enabled);

}

double SurfaceMesh::lengthScale() {

  // Measure length scale as twice the radius from the center of the bounding box
  auto bound = boundingBox(); 
  Vector3 center = 0.5 * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for(VertexPtr v : mesh->vertices()) {
    lengthScale = std::max(lengthScale, geometrycentral::norm2(geometry->position(v) - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
SurfaceMesh::boundingBox() {

  Vector3 min = Vector3{1,1,1}*std::numeric_limits<double>::infinity();
  Vector3 max = -Vector3{1,1,1}*std::numeric_limits<double>::infinity();

  for(VertexPtr v : mesh->vertices()) {
    min = geometrycentral::componentwiseMin(min, geometry->position(v));
    max = geometrycentral::componentwiseMax(max, geometry->position(v));
  }

  return {min, max};
}

}  // namespace polyscope