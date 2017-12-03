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
    glm::mat4 viewMat = view::getCameraViewMatrix();
    cameraSkeletonProgram->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

    glm::mat4 projMat = view::getCameraPerspectiveMatrix();
    cameraSkeletonProgram->setUniform("u_projMatrix", glm::value_ptr(projMat));

    cameraSkeletonProgram->setUniform("u_wirecolor", gl::RGB_BLACK);

    cameraSkeletonProgram->draw();
  }
}

void CameraView::drawPick() {}

// Helper function to convert glm::vec3 to Vector3
namespace {
Vector3 toV(glm::vec3 x) { return Vector3{x.x, x.y, x.z}; }
}  // namespace

Vector3 CameraView::location() { return toV(parameters.getPosition()); }

void CameraView::prepare() {}

void CameraView::prepareCameraSkeleton() {
  // Create the GL program
  cameraSkeletonProgram = new gl::GLProgram(
      &WIREFRAME_VERT_SHADER, &WIREFRAME_FRAG_SHADER, gl::DrawMode::Lines);

  // Relevant points in world space
  glm::vec3 root = parameters.getPosition();
  glm::vec3 lookDir = parameters.getLookDir();
  glm::vec3 upDir = parameters.getUpDir();
  glm::vec3 rightDir = parameters.getRightDir();

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

  // Add lines
  positions.push_back(toV(root));
  positions.push_back(toV(upperLeft));
  positions.push_back(toV(root));
  positions.push_back(toV(lowerLeft));
  positions.push_back(toV(root));
  positions.push_back(toV(upperRight));
  positions.push_back(toV(root));
  positions.push_back(toV(lowerRight));
  positions.push_back(toV(upperLeft));
  positions.push_back(toV(upperRight));
  positions.push_back(toV(upperRight));
  positions.push_back(toV(lowerRight));
  positions.push_back(toV(lowerRight));
  positions.push_back(toV(lowerLeft));

  // Show frame
  positions.push_back(toV(root));
  positions.push_back(toV(root + upDir * cameraDrawSize * 2.f));
  positions.push_back(toV(root));
  positions.push_back(toV(root + rightDir * cameraDrawSize));

  // Debug frame
  positions.push_back(toV(lowerLeft));
  positions.push_back(toV(upperLeft));

  // Store data in buffers
  cameraSkeletonProgram->setAttribute("a_position", positions);
}

void CameraView::drawUI() {
  
  if (ImGui::TreeNode(name.c_str())) {
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::SameLine();

    if (ImGui::Button("Fly to")) {
      view::startFlightTo(parameters, .3);
    }

    ImGui::TreePop();
  }
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