#include "polyscope/point_cloud.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

using namespace geometrycentral;
using std::cout; using std::endl;

namespace polyscope {

PointCloud::PointCloud(std::string name, const std::vector<Vector3>& points_)
    : Structure(name, StructureType::PointCloud), points(points_) {

  pointColor = getNextPaletteColor();

  prepare();
  preparePick();
}

PointCloud::~PointCloud() {
  if (program != nullptr) {
    delete program;
  }
}


// Helper to set uniforms
void PointCloud::setPointCloudBillboardUniforms(gl::GLProgram* p, bool withLight) {

  glm::mat4 viewMat = view::getCameraViewMatrix();
  p->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  p->setUniform("u_projMatrix", glm::value_ptr(projMat));

  if (withLight) {
    Vector3 eyePos = view::getCameraWorldPosition();
    p->setUniform("u_eye", eyePos);

    p->setUniform("u_lightCenter", state::center);
    p->setUniform("u_lightDist", 2 * state::lengthScale);
  }

  Vector3 lookDir, upDir, rightDir;
  view::getCameraFrame(lookDir, upDir, rightDir);

  p->setUniform("u_camZ", lookDir);
  p->setUniform("u_camUp", upDir);
  p->setUniform("u_camRight", rightDir);

  p->setUniform("u_pointRadius", pointRadius * state::lengthScale);
}

void PointCloud::draw() {
  if (!enabled) {
    return;
  }

  // Set uniforms
  setPointCloudBillboardUniforms(program, true);
  program->setUniform("u_color", pointColor);

  program->draw();
}

void PointCloud::drawPick() {

  if (!enabled) {
    return;
  }

  // Set uniforms
  setPointCloudBillboardUniforms(pickProgram, false);

  pickProgram->draw();
}

void PointCloud::prepare() {
  // Create the GL program
  // program = new gl::GLProgram(&PASSTHRU_SPHERE_VERT_SHADER, &SPHERE_GEOM_SHADER,
  //                             &SHINY_SPHERE_FRAG_SHADER, gl::DrawMode::Points);
  program = new gl::GLProgram(&PASSTHRU_SPHERE_VERT_SHADER, &SPHERE_GEOM_BILLBOARD_SHADER, &SHINY_SPHERE_FRAG_SHADER,
                              gl::DrawMode::Points);


  // Store data in buffers
  program->setAttribute("a_position", points);
}

void PointCloud::preparePick() {

  // Request pick indices
  size_t pickCount = points.size();
  size_t pickStart = pick::requestPickBufferRange(this, pickCount);

  // Create a new pick program
  safeDelete(pickProgram);
  pickProgram = new gl::GLProgram(&PASSTHRU_SPHERE_COLORED_VERT_SHADER, &SPHERE_GEOM_PLAIN_COLORED_BILLBOARD_SHADER,
                                  &PLAIN_SPHERE_COLORED_FRAG_SHADER, gl::DrawMode::Points);

  // Fill an index buffer
  std::vector<Vector3> pickColors;
  for (size_t i = pickStart; i < pickStart + pickCount; i++) {
    Vector3 val = pick::indToVec(i);
    pickColors.push_back(pick::indToVec(i));
  }


  // Store data in buffers
  pickProgram->setAttribute("a_position", points);
  pickProgram->setAttribute("a_color", pickColors);
}

void PointCloud::drawSharedStructureUI() {}

void PointCloud::drawPickUI(size_t localPickID) {

  ImGui::TextUnformatted(("#" + std::to_string(localPickID) + "  ").c_str());
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << points[localPickID];
  ImGui::TextUnformatted(buffer.str().c_str());

}

void PointCloud::drawUI() {

  ImGui::PushID(name.c_str()); // ensure there are no conflicts with identically-named labels

  ImGui::TextUnformatted(name.c_str());
  ImGui::Checkbox("Enabled", &enabled);
  ImGui::SameLine();
  ImGui::ColorEdit3("Point color", (float*)&pointColor, ImGuiColorEditFlags_NoInputs);

  ImGui::SliderFloat("Point Radius", &pointRadius, 0.0, .1, "%.5f", 3.);

  ImGui::PopID();

}

double PointCloud::lengthScale() {

  // Measure length scale as twice the radius from the center of the bounding box
  auto bound = boundingBox();
  Vector3 center = 0.5 * (std::get<0>(bound) + std::get<1>(bound));

  double lengthScale = 0.0;
  for (Vector3& p : points) {
    lengthScale = std::max(lengthScale, geometrycentral::norm2(p - center));
  }

  return 2 * std::sqrt(lengthScale);
}

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3> PointCloud::boundingBox() {

  Vector3 min = Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();
  Vector3 max = -Vector3{1, 1, 1} * std::numeric_limits<double>::infinity();

  for (Vector3& p : points) {
    min = geometrycentral::componentwiseMin(min, p);
    max = geometrycentral::componentwiseMax(max, p);
  }

  return std::make_tuple(min, max);
}

} // namespace polyscope