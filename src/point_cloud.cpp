#include "polyscope/point_cloud.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using namespace geometrycentral;

namespace polyscope {

PointCloud::PointCloud(std::string name, const std::vector<Vector3>& points_)
    : Structure(name, StructureType::PointCloud), points(points_) {

  pointColor = gl::RGB_ORANGE.toFloatArray();

  prepare();
}

void PointCloud::draw() {
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
  program->setUniform("u_lightDist", 2*state::lengthScale);
  
  program->setUniform("u_camZ", view::cameraDirection);
  program->setUniform("u_camUp", view::upDirection);
  Vector3 leftHand = unit(cross(view::cameraDirection, view::upDirection));
  program->setUniform("u_camRight", -leftHand); 
  
  program->setUniform("u_pointRadius", pointRadius * state::lengthScale);
  program->setUniform("u_color", pointColor);

  program->draw();
}

void PointCloud::drawPick() {}

void PointCloud::prepare() {
  // Create the GL program
  // program = new gl::GLProgram(&PASSTHRU_SPHERE_VERT_SHADER, &SPHERE_GEOM_SHADER,
  //                             &SHINY_SPHERE_FRAG_SHADER, gl::DrawMode::Points);
  program = new gl::GLProgram(&PASSTHRU_SPHERE_VERT_SHADER, &SPHERE_GEOM_BILLBOARD_SHADER,
                              &SHINY_SPHERE_FRAG_SHADER, gl::DrawMode::Points);

  // Constant color
  std::vector<Vector3> colorData(points.size());

  // Store data in buffers
  program->setAttribute("a_position", points);
}

void PointCloud::teardown() {
  if (program != nullptr) {
    delete program;
  }
}

void PointCloud::drawUI() {

  ImGui::PushID(name.c_str()); // ensure there are no conflicts with identically-named labels

  ImGui::TextUnformatted(name.c_str());
  ImGui::Checkbox("Enabled", &enabled);
  ImGui::ColorEdit3("Point color", (float*)&pointColor,
                    ImGuiColorEditFlags_NoInputs);

  ImGui::SliderFloat("Point Radius", &pointRadius, 0.0, .1, "%.5f", 3.);

  ImGui::PopID();
}

double PointCloud::lengthScale() {

  // Measure length scale as twice the radius from the center of the bounding box
  auto bound = boundingBox(); 
  Vector3 center = 0.5 * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for(Vector3& p : points) {
    lengthScale = std::max(lengthScale, geometrycentral::norm2(p - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
PointCloud::boundingBox() {

  Vector3 min = Vector3{1,1,1}*std::numeric_limits<double>::infinity();
  Vector3 max = -Vector3{1,1,1}*std::numeric_limits<double>::infinity();

  for(Vector3& p : points) {
    min = geometrycentral::componentwiseMin(min, p);
    max = geometrycentral::componentwiseMax(max, p);
  }

  return {min, max};
}

}  // namespace polyscope