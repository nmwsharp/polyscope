#include "polyscope/point_cloud.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"

using namespace geometrycentral;

namespace polyscope {

PointCloud::PointCloud(std::string name, const std::vector<Vector3>& points_)
    : Structure(name), points(points_) {
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

  Vector3 lightPos = view::getLightWorldPosition();
  program->setUniform("u_light", lightPos);

  program->draw();
}

void PointCloud::drawPick() {}

void PointCloud::prepare() {
  // Create the GL program
  program = new gl::GLProgram(&PASSTHRU_SPHERE_VERT_SHADER, &SPHERE_GEOM_SHADER,
                              &SHINY_SPHERE_FRAG_SHADER, gl::DrawMode::Points);

  // Constant color
  std::vector<Vector3> colorData(points.size());
  std::fill(colorData.begin(), colorData.end(), gl::RGB_ORANGE);

  // Constant size
  std::vector<double> sizeData(points.size());
  std::fill(sizeData.begin(), sizeData.end(), .01);

  // Store data in buffers
  program->setAttribute("a_position", points);
  program->setAttribute("a_color", colorData);
  program->setAttribute("a_pointRadius", sizeData);
}

void PointCloud::teardown() {
  if (program != nullptr) {
    delete program;
  }
}

void PointCloud::drawUI() {}

double PointCloud::lengthScale() {
  return 1;  // TODO
}

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
PointCloud::boundingBox() {
  return {Vector3{0, 0, 0}, Vector3{1, 1, 1}};  // TODO
}

}  // namespace polyscope