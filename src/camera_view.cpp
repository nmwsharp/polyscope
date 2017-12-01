#include "polyscope/camera_view.h"

#include "polyscope/gl/colors.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/wireframe_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include "glm/glm.hpp"

using namespace geometrycentral;

namespace polyscope {

CameraView::CameraView(std::string name, CameraParameters p_)
    : Structure(name, StructureType::CameraView), parameters(p_) {
  prepareCameraSkeleton();
}

CameraView::~CameraView() {
  if (cameraSkeletonProgram != nullptr) {
    delete cameraSkeletonProgram;
  }
}

void CameraView::draw() {
  if (!enabled) {
    return;
  }

  {  // Camera skeleton program

    if (cameraSkeletonProgram == nullptr) {
      prepareCameraSkeleton();
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set uniforms
    glm::mat4 viewMat = view::getViewMatrix();
    cameraSkeletonProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

    glm::mat4 projMat = view::getPerspectiveMatrix();
    cameraSkeletonProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));

    cameraSkeletonProgram->setUniform("u_edgeWidth",
                                      0.001 * state::lengthScale);
    cameraSkeletonProgram->setUniform("u_wirecolor", gl::RGB_BLACK);

    cameraSkeletonProgram->draw();
  }
}

void CameraView::drawPick() {}

// Helper function to convert glm::vec3 to Vector3
namespace {
Vector3 toV(glm::vec3 x) { return Vector3{x.x, x.y, x.z}; }
}  // namespace

Vector3 CameraView::location() {
  return toV(-glm::transpose(parameters.R) * parameters.T);
}

void CameraView::prepare() {}

void CameraView::prepareCameraSkeleton() {
  // Create the GL program
  cameraSkeletonProgram = new gl::GLProgram(
      &WIREFRAME_VERT_SHADER, &WIREFRAME_FRAG_SHADER, gl::DrawMode::Triangles);

  // Relevant points in world space
  glm::vec3 root = -glm::transpose(parameters.R) * parameters.T;
  glm::vec3 lookDir = glm::normalize(parameters.R * glm::vec3(0.0, 0.0, 1.0));
  glm::vec3 upDir = -glm::normalize(parameters.R * glm::vec3(0.0, 1.0, 0.0));
  glm::vec3 rightDir = -glm::cross(lookDir, upDir);

  float cameraDrawSize = state::lengthScale * 0.1;
  float frameDrawWidth = 0.5 / parameters.focalLengths.x * cameraDrawSize;
  float frameDrawHeight = 0.5 / parameters.focalLengths.y * cameraDrawSize;

  glm::vec3 upperLeft = root + cameraDrawSize * lookDir +
                        upDir * frameDrawHeight - rightDir * frameDrawWidth;
  glm::vec3 lowerLeft = root + cameraDrawSize * lookDir -
                        upDir * frameDrawHeight - rightDir * frameDrawWidth;
  glm::vec3 upperRight = root + cameraDrawSize * lookDir +
                         upDir * frameDrawHeight + rightDir * frameDrawWidth;
  glm::vec3 lowerRight = root + cameraDrawSize * lookDir -
                         upDir * frameDrawHeight + rightDir * frameDrawWidth;

  // Triangles to draw
  std::vector<Vector3> positions;  // position in space
  std::vector<Vector3> edgeDists;  // distance to edge opposite this vertex

  // Helper to add triangle with edge dist
  auto addTriangle = [&](std::array<glm::vec3, 3> points) {
    for (int i = 0; i < 3; i++) {
      // Add the points
      positions.push_back(toV(points[i]));

      // Add edge dist
      glm::vec3 v1 = points[(i + 2) % 3] - points[(i + 1) % 3];
      glm::vec3 v2 = points[(i + 0) % 3] - points[(i + 1) % 3];
      glm::vec3 v1n = glm::normalize(v1);
      double d = glm::length(v2 - glm::dot(v2, v1n) * v1n);
      cout << "d = " << d << endl;
      Vector3 dists{0.0, 0.0, 0.0};
      dists[i] = d;
      edgeDists.push_back(dists);
    }
  };

  // Add triangles
  addTriangle({{root, upperLeft, lowerLeft}});
  addTriangle({{root, upperRight, upperLeft}});
  addTriangle({{root, lowerRight, upperRight}});
  addTriangle({{root, lowerLeft, lowerRight}});

  // Store data in buffers
  cameraSkeletonProgram->setAttribute("a_position", positions);
  cameraSkeletonProgram->setAttribute("a_edgeDists", edgeDists);
}

void CameraView::drawUI() {
  ImGui::PushID(name.c_str());  // ensure there are no conflicts with
                                // identically-named labels

  ImGui::TextUnformatted(name.c_str());
  ImGui::Checkbox("Enabled", &enabled);
  // ImGui::SameLine();
  // ImGui::ColorEdit3("Point color", (float*)&pointColor,
  //                   ImGuiColorEditFlags_NoInputs);

  // ImGui::SliderFloat("Point Radius", &pointRadius, 0.0, .1, "%.5f", 3.);

  ImGui::PopID();
}

double CameraView::lengthScale() {
  // Measure length scale as twice the radius from the center of the bounding
  // box

  return 0;
}

std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
CameraView::boundingBox() {
  Vector3 pos = location();
  return std::make_tuple(pos, pos);
}

}  // namespace polyscope