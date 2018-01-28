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
double fov = 65.0;
double nearClipRatio = 0.005;
double farClipRatio = 20.0;
std::array<float, 4> bgColor{{.88, .88, .88, 1.0}};

glm::mat4x4 viewMat;

bool midflight = false;
float flightStartTime = -1;
float flightEndTime = -1;
glm::dualquat flightTargetViewR, flightInitialViewR;
glm::vec3 flightTargetViewT, flightInitialViewT;
float flightTargetFov, flightInitialFov;


void processRotate(float delTheta, float delPhi) {
  if (delTheta == 0 && delPhi == 0) {
    return;
  }

  // Scaling
  delTheta *= PI;
  delPhi *= PI;

  // Get frame
  Vector3 lookDir, upDir, rightDir;
  getCameraFrame(lookDir, upDir, rightDir);
  glm::vec3 upGLM(upDir.x, upDir.y, upDir.z);
  glm::vec3 rightGLM(rightDir.x, rightDir.y, rightDir.z);

  // Rotation about the vertical axis
  glm::mat4x4 thetaCamR = glm::rotate(glm::mat4x4(1.0), delTheta, upGLM);
  viewMat = viewMat * thetaCamR;

  // Rotation about the horizontal axis
  glm::mat4x4 phiCamR = glm::rotate(glm::mat4x4(1.0), -delPhi, rightGLM);
  viewMat = viewMat * phiCamR;

  immediatelyEndFlight();
}

void processRotateArcball(Vector2 startP, Vector2 endP) {

  if (endP == startP) {
    return;
  }

  // Map inputs to unit sphere
  auto toSphere = [](Vector2 v) {
    double x = clamp(v.x, -1.0, 1.0);
    double y = clamp(v.y, -1.0, 1.0);
    double mag = x * x + y * y;
    if (mag <= 1.0) {
      return Vector3{x, y, -std::sqrt(1.0 - mag)};
    } else {
      return unit(Vector3{x, y, 0.0});
    }
  };
  Vector3 sphereStart = toSphere(startP);
  Vector3 sphereEnd = toSphere(endP);

  Vector3 rotAxis = -cross(sphereStart, sphereEnd);
  double rotMag = std::acos(clamp(dot(sphereStart, sphereEnd), -1.0, 1.0));
  
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

  immediatelyEndFlight();
}

void processTranslate(Vector2 delta) {
  if (norm(delta) == 0) {
    return;
  }
  // Process a translation
  float movementScale = state::lengthScale * 0.6;
  glm::mat4x4 camSpaceT = glm::translate(glm::mat4x4(1.0), movementScale * glm::vec3(delta.x, delta.y, 0.0));
  viewMat = camSpaceT * viewMat;

  immediatelyEndFlight();
}

void processClipPlaneShift(double amount) {
  // Adjust the near clipping plane
  nearClipRatio += .03 * amount * nearClipRatio;
}

void processZoom(double amount) {
  // Translate the camera forwards and backwards
  float movementScale = state::lengthScale * 0.1;
  glm::mat4x4 camSpaceT = glm::translate(glm::mat4x4(1.0), glm::vec3(0., 0., movementScale * amount));
  viewMat = camSpaceT * viewMat;

  immediatelyEndFlight();
}

void resetCameraToDefault() {

  // WARNING: Duplicated here and in flyToDefault()

  viewMat = glm::mat4x4(1.0);
  viewMat[0][0] = -1.;
  viewMat[2][2] = -1.;
  viewMat = viewMat * glm::translate(glm::mat4x4(1.0), glm::vec3(0.0, 0.0, state::lengthScale));

  fov = 65.0;
  nearClipRatio = 0.005;
  farClipRatio = 20.0;
}


void flyToDefault() {

  // WARNING: Duplicated here and in resetCameraToDefault()

  glm::mat4x4 T(1.0);
  T[0][0] = -1.;
  T[2][2] = -1.;
  T = T * glm::translate(glm::mat4x4(1.0), glm::vec3(0.0, 0.0, state::lengthScale));


  float Tfov = 65.0;
  nearClipRatio = 0.005;
  farClipRatio = 20.0;

  startFlightTo(T, Tfov, .25);
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

Vector3 getCameraWorldPosition() {
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
