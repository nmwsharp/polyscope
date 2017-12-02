#include "polyscope/view.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {
namespace view {

// Storage for state variables
int windowWidth = -1;
int windowHeight = -1;
int bufferWidth = -1;
int bufferHeight = -1;
double fov = 65.0;
double nearClipRatio = 0.01;
double farClipRatio = 20.0;
std::array<float, 4> bgColor{{.7, .7, .7, 1.0}};

glm::mat4x4 viewMat;

bool midflight = false;
float flightStartTime = -1;
float flightEndTime = -1;
glm::mat4x4 flightTargetView, flightInitialView;
float flightTargetFov, flightInitialFov;

void processMouseDrag(Vector2 deltaDrag, bool isRotating) {
  if (norm(deltaDrag) == 0) {
    return;
  }

  // Process a rotation
  if (isRotating) {
    float delTheta = deltaDrag.x * PI;
    float delPhi = deltaDrag.y * PI;

    // Rotation about the vertical axis
    glm::mat4x4 thetaCamR =
        glm::rotate(glm::mat4x4(1.0), delTheta, glm::vec3(0.0, 1.0, 0.0));
    viewMat = viewMat * thetaCamR;

    // Rotation about the horizontal axis
    // Get the "right" direction
    glm::mat3x3 R;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        R[i][j] = viewMat[i][j];
      }
    }
    glm::vec3 rotAx = glm::transpose(R) * glm::vec3(-1.0, 0.0, 0.0);
    glm::mat4x4 phiCamR = glm::rotate(glm::mat4x4(1.0), delPhi, rotAx);
    viewMat = viewMat * phiCamR;
  }
  // Process a translation
  else {
    float movementScale = state::lengthScale * 0.6;
    glm::mat4x4 camSpaceT = glm::translate(
        glm::mat4x4(1.0),
        movementScale * glm::vec3(deltaDrag.x, deltaDrag.y, 0.0));
    viewMat = camSpaceT * viewMat;
  }
}

void processMouseScroll(double scrollAmount, bool scrollClipPlane) {
  // Adjust the near clipping plane
  if (scrollClipPlane) {
    nearClipRatio += .03 * scrollAmount * nearClipRatio;
  }
  // Translate the camera forwards and backwards
  else {
    float movementScale = state::lengthScale * 0.1;
    glm::mat4x4 camSpaceT = glm::translate(
        glm::mat4x4(1.0),
        movementScale * glm::vec3(0., 0., movementScale * scrollAmount));
    viewMat = camSpaceT * viewMat;
  }
}

void resetCameraToDefault() {
  viewMat = glm::mat4x4(1.0);
  viewMat = viewMat * glm::translate(glm::mat4x4(1.0),
                                     glm::vec3(0.0, 0.0, state::lengthScale));

  fov = 65.0;
  // aspectRatio = 1.0;
  nearClipRatio = 0.01;
  farClipRatio = 20.0;
}

void setViewToCamera(const CameraParameters& p) {
  viewMat = p.E;
  fov = glm::degrees(2 * std::atan(1. / (2. * p.focalLengths.y)));
  // aspectRatio = p.focalLengths.x / p.focalLengths.y; // TODO should be
  // flipped?
}

glm::mat4 getCameraViewMatrix() {
  updateFlight();

  return viewMat;
}

glm::mat4 getCameraPerspectiveMatrix() {
  updateFlight();

  double farClip = farClipRatio * state::lengthScale;
  double nearClip = nearClipRatio * state::lengthScale;
  double fovRad = glm::radians(fov);
  double aspectRatio = (float)bufferWidth / bufferHeight;

  return glm::perspective(fovRad, aspectRatio, nearClip, farClip);
}

Vector3 getCameraWorldPosition() {
  // return lookAtPoint + cameraDirection * dist;

  // This will work no matter how the view matrix is constructed...
  glm::mat4 invViewMat = inverse(getCameraViewMatrix());
  return Vector3{invViewMat[3][0], invViewMat[3][1], invViewMat[3][2]};
}

void getCameraFrame(Vector3& lookDir, Vector3& upDir, Vector3& rightDir) {
  glm::mat3x3 R;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      R[i][j] = viewMat[i][j];
    }
  }
  glm::mat3x3 Rt = glm::transpose(R);

  lookDir = fromGLM(Rt * glm::vec3(0.0, 0.0, -1.0));
  upDir = fromGLM(Rt * glm::vec3(0.0, 1.0, 0.0));
  rightDir = fromGLM(Rt * glm::vec3(1.0, 0.0, 0.0));
}

void startFlightTo(const CameraParameters& p, float flightLengthInSeconds) {

  flightStartTime = ImGui::GetTime();
  flightEndTime = ImGui::GetTime() + flightLengthInSeconds;

  flightInitialView = getCameraViewMatrix();
  flightInitialFov = fov;

  flightTargetView = p.E;
  flightTargetFov = glm::degrees(2 * std::atan(1. / (2. * p.focalLengths.y)));
  
  midflight = true;
}

void immediatelyEndFlight() { midflight = false; }

void updateFlight() {
  if (midflight) {
    if (ImGui::GetTime() > flightEndTime) {
      // Flight is over, ensure we end exactly at target location
      midflight = false;
      viewMat = flightTargetView;
      fov = flightTargetFov;
    } else {
      // normalized time for spline on [0,1]
      float t = (ImGui::GetTime() - flightStartTime) /
                (flightEndTime - flightStartTime);

      // linear spline for now
      viewMat = (1.0f - t) * flightInitialView + t * flightTargetView;
      fov = (1.0f - t) * flightInitialFov + t * flightTargetFov;
    }
  }
}

}  // namespace view
}  // namespace polyscope
