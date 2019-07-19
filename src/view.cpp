// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/view.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {
namespace view {

// Storage for state variables
int windowWidth = 1280;
int windowHeight = 720;
int bufferWidth = -1;
int bufferHeight = -1;
int initWindowPosX = 20;
int initWindowPosY = 20;
NavigateStyle style = NavigateStyle::Turntable;
double moveScale = 1.0;
const double defaultNearClipRatio = 0.005;
const double defaultFarClipRatio = 20.0;
const double defaultFov = 65.;
double fov = defaultFov;
double nearClipRatio = defaultNearClipRatio;
double farClipRatio = defaultFarClipRatio;
std::array<float, 4> bgColor{{1.0, 1.0, 1.0, 1.0}};

glm::mat4x4 viewMat;

bool midflight = false;
float flightStartTime = -1;
float flightEndTime = -1;
glm::dualquat flightTargetViewR, flightInitialViewR;
glm::vec3 flightTargetViewT, flightInitialViewT;
float flightTargetFov, flightInitialFov;


void processRotate(glm::vec2 startP, glm::vec2 endP) {

  if (startP == endP) {
    return;
  }

  // Get frame
  glm::vec3 lookDir, upDir, rightDir;
  getCameraFrame(lookDir, upDir, rightDir);

  switch (style) {
  case NavigateStyle::Turntable: {

    glm::vec2 dragDelta = endP - startP;
    float delTheta = 2.0 * dragDelta.x * moveScale;
    float delPhi = 2.0 * dragDelta.y * moveScale;

    // Translate to center
    viewMat = glm::translate(viewMat, state::center);

    // Rotation about the horizontal axis
    glm::mat4x4 phiCamR = glm::rotate(glm::mat4x4(1.0), -delPhi, rightDir);
    viewMat = viewMat * phiCamR;

    // Rotation about the vertical axis
    glm::mat4x4 thetaCamR = glm::rotate(glm::mat4x4(1.0), delTheta, glm::vec3(0., 1., 0.));
    viewMat = viewMat * thetaCamR;

    // Undo centering
    viewMat = glm::translate(viewMat, -state::center);
    break;
  }
  case NavigateStyle::Free: {
    glm::vec2 dragDelta = endP - startP;
    float delTheta = 2.0 * dragDelta.x * moveScale;
    float delPhi = 2.0 * dragDelta.y * moveScale;

    // Translate to center
    viewMat = glm::translate(viewMat, state::center);

    // Rotation about the vertical axis
    glm::mat4x4 thetaCamR = glm::rotate(glm::mat4x4(1.0), delTheta, upDir);
    viewMat = viewMat * thetaCamR;

    // Rotation about the horizontal axis
    glm::mat4x4 phiCamR = glm::rotate(glm::mat4x4(1.0), -delPhi, rightDir);
    viewMat = viewMat * phiCamR;

    // Undo centering
    viewMat = glm::translate(viewMat, -state::center);
    break;
  }
  case NavigateStyle::Planar: {
    // Do nothing
    break;
  }
  case NavigateStyle::Arcball: {
    // Map inputs to unit sphere
    auto toSphere = [](glm::vec2 v) {
      double x = glm::clamp(v.x, -1.0f, 1.0f);
      double y = glm::clamp(v.y, -1.0f, 1.0f);
      double mag = x * x + y * y;
      if (mag <= 1.0) {
        return glm::vec3{x, y, -std::sqrt(1.0 - mag)};
      } else {
        return glm::normalize(glm::vec3{x, y, 0.0});
      }
    };
    glm::vec3 sphereStart = toSphere(startP);
    glm::vec3 sphereEnd = toSphere(endP);

    glm::vec3 rotAxis = -cross(sphereStart, sphereEnd);
    double rotMag = std::acos(glm::clamp(dot(sphereStart, sphereEnd), -1.0f, 1.0f) * moveScale);

    glm::mat4 cameraRotate = glm::rotate(glm::mat4x4(1.0), (float)rotMag, glm::vec3(rotAxis.x, rotAxis.y, rotAxis.z));

    // Get current camera rotation
    glm::mat4x4 R;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        R[i][j] = viewMat[i][j];
      }
    }
    R[3][3] = 1.0;

    glm::mat4 update = glm::inverse(R) * cameraRotate * R;

    viewMat = viewMat * update;
    break;
  }
  }


  requestRedraw();
  immediatelyEndFlight();
}

void processRotateArcball(glm::vec2 startP, glm::vec2 endP) {

  if (endP == startP) {
    return;
  }


  requestRedraw();
  immediatelyEndFlight();
}

void processTranslate(glm::vec2 delta) {
  if (glm::length(delta) == 0) {
    return;
  }
  // Process a translation
  float movementScale = state::lengthScale * 0.6 * moveScale;
  glm::mat4x4 camSpaceT = glm::translate(glm::mat4x4(1.0), movementScale * glm::vec3(delta.x, delta.y, 0.0));
  viewMat = camSpaceT * viewMat;

  requestRedraw();
  immediatelyEndFlight();
}

void processClipPlaneShift(double amount) {
  if (amount == 0.0) return;
  // Adjust the near clipping plane
  nearClipRatio += .03 * amount * nearClipRatio;
  requestRedraw();
}

void processZoom(double amount) {
  if (amount == 0.0) return;

  // Translate the camera forwards and backwards
  float movementScale = state::lengthScale * 0.1 * moveScale;
  glm::mat4x4 camSpaceT = glm::translate(glm::mat4x4(1.0), glm::vec3(0., 0., movementScale * amount));
  viewMat = camSpaceT * viewMat;

  immediatelyEndFlight();
  requestRedraw();
}

void invalidateView() { viewMat = glm::mat4x4(std::numeric_limits<float>::quiet_NaN()); }

void ensureViewValid() {
  bool allFinite = true;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (!std::isfinite(viewMat[i][j])) {
        allFinite = false;
      }
    }
  }

  if (!allFinite) {
    resetCameraToHomeView();
  }
}

void resetCameraToDefault() {

  // WARNING: Duplicated here and in flyToDefault()

  viewMat = glm::mat4x4(1.0);
  viewMat[0][0] = -1.;
  viewMat[2][2] = -1.;
  viewMat = viewMat * glm::translate(glm::mat4x4(1.0), glm::vec3(0.0, 0.0, state::lengthScale));

  fov = defaultFov;
  nearClipRatio = defaultNearClipRatio;
  farClipRatio = defaultFarClipRatio;

  requestRedraw();
}

void flyToDefault() {

  // WARNING: Duplicated here and in resetCameraToDefault()

  glm::mat4x4 T(1.0);
  T[0][0] = -1.;
  T[2][2] = -1.;
  T = T * glm::translate(glm::mat4x4(1.0), glm::vec3(0.0, 0.0, state::lengthScale));


  float Tfov = defaultFov;
  nearClipRatio = defaultNearClipRatio;
  farClipRatio = defaultFarClipRatio;

  startFlightTo(T, Tfov, .4);
}

glm::mat4 computeHomeView() {

  glm::mat4x4 T(1.0);
  T[0][0] = -1.;
  T[2][2] = -1.;
  T = T *
      glm::translate(glm::mat4x4(1.0), -state::center + glm::vec3(0.0, -0.1 * state::lengthScale, state::lengthScale));

  return T;
}

void resetCameraToHomeView() {

  // WARNING: Duplicated here and in flyToHomeView()

  viewMat = computeHomeView();

  fov = defaultFov;
  nearClipRatio = defaultNearClipRatio;
  farClipRatio = defaultFarClipRatio;

  requestRedraw();
}

void flyToHomeView() {

  // WARNING: Duplicated here and in resetCameraToHomeView()

  glm::mat4x4 T = computeHomeView();

  float Tfov = defaultFov;
  nearClipRatio = defaultNearClipRatio;
  farClipRatio = defaultFarClipRatio;

  startFlightTo(T, Tfov, .4);
}


void setViewToCamera(const CameraParameters& p) {
  viewMat = p.E;
  // fov = glm::degrees(2 * std::atan(1. / (2. * p.focalLengths.y)));
  fov = p.fov;
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

glm::vec3 getCameraWorldPosition() {
  // This will work no matter how the view matrix is constructed...
  glm::mat4 invViewMat = inverse(getCameraViewMatrix());
  return glm::vec3{invViewMat[3][0], invViewMat[3][1], invViewMat[3][2]};
}

void getCameraFrame(glm::vec3& lookDir, glm::vec3& upDir, glm::vec3& rightDir) {
  glm::mat3x3 R;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      R[i][j] = viewMat[i][j];
    }
  }
  glm::mat3x3 Rt = glm::transpose(R);

  lookDir = Rt * glm::vec3(0.0, 0.0, -1.0);
  upDir = Rt * glm::vec3(0.0, 1.0, 0.0);
  rightDir = Rt * glm::vec3(1.0, 0.0, 0.0);
}

void startFlightTo(const CameraParameters& p, float flightLengthInSeconds) {
  // startFlightTo(p.E, glm::degrees(2 * std::atan(1. / (2. * p.focalLengths.y))),
  //               flightLengthInSeconds);
  startFlightTo(p.E, p.fov, flightLengthInSeconds);
}

void startFlightTo(const glm::mat4& T, float targetFov, float flightLengthInSeconds) {
  flightStartTime = ImGui::GetTime();
  flightEndTime = ImGui::GetTime() + flightLengthInSeconds;

  // Initial parameters
  glm::mat3x4 Rstart;
  glm::vec3 Tstart;
  splitTransform(getCameraViewMatrix(), Rstart, Tstart);
  flightInitialViewR = glm::dualquat_cast(Rstart);
  flightInitialViewT = Tstart;
  flightInitialFov = fov;

  // Final parameters
  glm::mat3x4 Rend;
  glm::vec3 Tend;
  splitTransform(T, Rend, Tend);
  flightTargetViewR = glm::dualquat_cast(Rend);
  flightTargetViewT = Tend;
  flightTargetFov = targetFov;

  midflight = true;
}

void immediatelyEndFlight() { midflight = false; }

void updateFlight() {
  if (midflight) {
    if (ImGui::GetTime() > flightEndTime) {
      // Flight is over, ensure we end exactly at target location
      midflight = false;
      viewMat = buildTransform(glm::mat3x4_cast(flightTargetViewR), flightTargetViewT);
      fov = flightTargetFov;
    } else {
      // normalized time for spline on [0,1]
      float t = (ImGui::GetTime() - flightStartTime) / (flightEndTime - flightStartTime);

      float tSmooth = glm::smoothstep(0.f, 1.f, t);

      // linear spline
      glm::dualquat interpR = glm::lerp(flightInitialViewR, flightTargetViewR, tSmooth);

      glm::vec3 interpT = glm::mix(flightInitialViewT, flightTargetViewT, tSmooth);

      viewMat = buildTransform(glm::mat3x4_cast(interpR), interpT);

      // linear spline
      fov = (1.0f - t) * flightInitialFov + t * flightTargetFov;
    }
    requestRedraw();
  }
}

void splitTransform(const glm::mat4& trans, glm::mat3x4& R, glm::vec3& T) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      R[i][j] = trans[i][j];
    }
    T[i] = trans[3][i];
  }
}

glm::mat4 buildTransform(const glm::mat3x4& R, const glm::vec3& T) {
  glm::mat4 trans(1.0);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      trans[i][j] = R[i][j];
    }
    trans[3][i] = T[i];
  }

  return trans;
}

} // namespace view
} // namespace polyscope
