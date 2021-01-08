// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/view.h"

#include "polyscope/polyscope.h"
#include "polyscope/utilities.h"

#include "imgui.h"

#include "json/json.hpp"
using json = nlohmann::json;

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
UpDir upDir = UpDir::YUp;
double moveScale = 1.0;
const double defaultNearClipRatio = 0.005;
const double defaultFarClipRatio = 20.0;
const double defaultFov = 45.;
double fov = defaultFov;
double nearClipRatio = defaultNearClipRatio;
double farClipRatio = defaultFarClipRatio;
std::array<float, 4> bgColor{{1.0, 1.0, 1.0, 0.0}};

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
  glm::vec3 frameLookDir, frameUpDir, frameRightDir;
  getCameraFrame(frameLookDir, frameUpDir, frameRightDir);

  switch (style) {
  case NavigateStyle::Turntable: {

    glm::vec2 dragDelta = endP - startP;
    float delTheta = 2.0 * dragDelta.x * moveScale;
    float delPhi = 2.0 * dragDelta.y * moveScale;

    // Translate to center
    viewMat = glm::translate(viewMat, state::center);

    // Rotation about the horizontal axis
    glm::mat4x4 phiCamR = glm::rotate(glm::mat4x4(1.0), -delPhi, frameRightDir);
    viewMat = viewMat * phiCamR;

    // Rotation about the vertical axis
    glm::vec3 turntableUp;
    switch (upDir) {
    case UpDir::XUp:
      turntableUp = glm::vec3(1., 0., 0.);
      break;
    case UpDir::YUp:
      turntableUp = glm::vec3(0., 1., 0.);
      break;
    case UpDir::ZUp:
      turntableUp = glm::vec3(0., 0., 1.);
      break;
    case UpDir::NegXUp:
      turntableUp = glm::vec3(-1., 0., 0.);
      break;
    case UpDir::NegYUp:
      turntableUp = glm::vec3(0., -1., 0.);
      break;
    case UpDir::NegZUp:
      turntableUp = glm::vec3(0., 0., -1.);
      break;
    }
    glm::mat4x4 thetaCamR = glm::rotate(glm::mat4x4(1.0), delTheta, turntableUp);
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
    glm::mat4x4 thetaCamR = glm::rotate(glm::mat4x4(1.0), delTheta, frameUpDir);
    viewMat = viewMat * thetaCamR;

    // Rotation about the horizontal axis
    glm::mat4x4 phiCamR = glm::rotate(glm::mat4x4(1.0), -delPhi, frameRightDir);
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

bool viewIsValid() {
  bool allFinite = true;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (!std::isfinite(viewMat[i][j])) {
        allFinite = false;
      }
    }
  }
  return allFinite;
}

void ensureViewValid() {
  if (!viewIsValid()) {
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        if (!std::isfinite(viewMat[i][j])) {
          viewMat[i][j] = 0.;
        }
      }
    }
    resetCameraToHomeView();
  }
}

glm::mat4 computeHomeView() {

  glm::mat4x4 R(1.0);
  glm::vec3 baseUp;
  switch (upDir) {
  case UpDir::XUp:
  case UpDir::NegXUp:
    baseUp = glm::vec3(1., 0., 0.);
    R = glm::rotate(glm::mat4x4(1.0), static_cast<float>(PI / 2), glm::vec3(0., 0., 1.));
    if (upDir == UpDir::NegXUp) {
      baseUp *= -1;
      R = glm::rotate(R, static_cast<float>(PI), glm::vec3(0., 0., 1.));
    }
    break;
  case UpDir::YUp:
  case UpDir::NegYUp:
    baseUp = glm::vec3(0., 1., 0.);
    // this is our camera's default
    if (upDir == UpDir::NegYUp) {
      baseUp *= -1;
      R = glm::rotate(R, static_cast<float>(PI), glm::vec3(0., 0., 1.));
    }
    break;
  case UpDir::ZUp:
  case UpDir::NegZUp:
    baseUp = glm::vec3(0., 0., 1.);
    R = glm::rotate(glm::mat4x4(1.0), static_cast<float>(PI / 2), glm::vec3(-1., 0., 0.));
    R = glm::rotate(glm::mat4x4(1.0), static_cast<float>(PI), glm::vec3(0., 1., 0.)) *
        R; // follow common convention for "front"
    if (upDir == UpDir::NegZUp) {
      baseUp *= -1;
      R = glm::rotate(R, static_cast<float>(PI), glm::vec3(0., 1., 0.));
    }
    break;
  }

  // Rotate around the up axis, since our camera looks down -Z
  R = glm::rotate(R, static_cast<float>(PI), baseUp);

  glm::mat4x4 Tobj = glm::translate(glm::mat4x4(1.0), -state::center);
  glm::mat4x4 Tcam =
      glm::translate(glm::mat4x4(1.0), glm::vec3(0.0, -0.1 * state::lengthScale, -1.5 * state::lengthScale));

  return Tcam * R * Tobj;
}

void resetCameraToHomeView() {

  // WARNING: Duplicated here and in flyToHomeView()

  // If the view is invalid, don't change it. It will get reset before the first call to show().
  if (!viewIsValid()) {
    return;
  }

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

  startFlightTo(T, Tfov);
}


void setViewToCamera(const CameraParameters& p) {
  viewMat = p.E;
  // fov = glm::degrees(2 * std::atan(1. / (2. * p.focalLengths.y)));
  fov = p.fov;
  // aspectRatio = p.focalLengths.x / p.focalLengths.y; // TODO should be
  // flipped?
}

glm::mat4 getCameraViewMatrix() { return viewMat; }

glm::mat4 getCameraPerspectiveMatrix() {
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

glm::vec3 screenCoordsToWorldRay(glm::vec2 screenCoords) {

  glm::mat4 view = getCameraViewMatrix();
  glm::mat4 proj = getCameraPerspectiveMatrix();
  glm::vec4 viewport = {0., 0., view::windowWidth, view::windowHeight};

  glm::vec3 screenPos3{screenCoords.x, view::windowHeight - screenCoords.y, 0.};
  glm::vec3 worldPos = glm::unProject(screenPos3, view, proj, viewport);
  glm::vec3 worldRayDir = glm::normalize(glm::vec3(worldPos) - getCameraWorldPosition());

  return worldRayDir;
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
    requestRedraw(); // flight is still happening, draw again next frame
  }
}

std::string getCameraJson() {

  // Get the view matrix (note weird glm indexing, glm is [col][row])
  glm::mat4 viewMat = getCameraViewMatrix();
  std::array<double, 16> viewMatFlat;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      viewMatFlat[4 * i + j] = viewMat[j][i];
    }
  }

  // Build the json object
  json j = {
      {"fov", fov},
      {"viewMat", viewMatFlat},
      {"nearClipRatio", nearClipRatio},
      {"farClipRatio", farClipRatio},
  };

  std::string outString = j.dump();
  return outString;
}

void setCameraFromJson(std::string jsonData, bool flyTo) {
  // Values will go here
  glm::mat4 newViewMat;
  double newFov = -777;
  double newNearClipRatio = -777;
  double newFarClipRatio = -777;

  try {

    // Deserialize
    json j;
    std::stringstream s(jsonData);
    s >> j;

    // Read out the data
    auto matData = j["viewMat"];
    if (matData.size() != 16) return; // check size
    auto it = matData.begin();
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        newViewMat[j][i] = *it;
        it++;
      }
    }
    newFov = j["fov"];

    // Get the clip ratios, but only if present
    if (j.find("nearClipRatio") != j.end()) {
      newNearClipRatio = j["nearClipRatio"];
    }
    if (j.find("farClipRatio") != j.end()) {
      newFarClipRatio = j["farClipRatio"];
    }

  } catch (...) {
    // If anything goes wrong parsing, just give up
    return;
  }

  // Assign the new values
  if (newNearClipRatio > 0) nearClipRatio = newNearClipRatio;
  if (newFarClipRatio > 0) farClipRatio = newFarClipRatio;

  if (flyTo) {
    startFlightTo(newViewMat, fov);
  } else {
    viewMat = newViewMat;
    fov = newFov;
    requestRedraw();
  }
}

void buildViewGui() {

  ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
  if (ImGui::TreeNode("View")) {

    // == Camera style

    std::string viewStyleName;
    switch (view::style) {
    case view::NavigateStyle::Turntable:
      viewStyleName = "Turntable";
      break;
    case view::NavigateStyle::Free:
      viewStyleName = "Free";
      break;
    case view::NavigateStyle::Planar:
      viewStyleName = "Planar";
      break;
    case view::NavigateStyle::Arcball:
      viewStyleName = "Arcball";
      break;
    }

    ImGui::PushItemWidth(120);
    if (ImGui::BeginCombo("##View Style", viewStyleName.c_str())) {
      if (ImGui::Selectable("Turntable", view::style == view::NavigateStyle::Turntable)) {
        view::style = view::NavigateStyle::Turntable;
        view::flyToHomeView();
        ImGui::SetItemDefaultFocus();
      }
      if (ImGui::Selectable("Free", view::style == view::NavigateStyle::Free)) {
        view::style = view::NavigateStyle::Free;
        ImGui::SetItemDefaultFocus();
      }
      if (ImGui::Selectable("Planar", view::style == view::NavigateStyle::Planar)) {
        view::style = view::NavigateStyle::Planar;
        view::flyToHomeView();
        ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();

    ImGui::Text("Camera Style");

    // == Up direction
    ImGui::PushItemWidth(120);
    std::string upStyleName;
    switch (upDir) {
    case UpDir::XUp:
      upStyleName = "X Up";
      break;
    case UpDir::NegXUp:
      upStyleName = "-X Up";
      break;
    case UpDir::YUp:
      upStyleName = "Y Up";
      break;
    case UpDir::NegYUp:
      upStyleName = "-Y Up";
      break;
    case UpDir::ZUp:
      upStyleName = "Z Up";
      break;
    case UpDir::NegZUp:
      upStyleName = "-Z Up";
      break;
    }

    if (ImGui::BeginCombo("##Up Direction", upStyleName.c_str())) {
      if (ImGui::Selectable("X Up", view::upDir == view::UpDir::XUp)) {
        view::setUpDir(view::UpDir::XUp, true);
        ImGui::SetItemDefaultFocus();
      }
      if (ImGui::Selectable("-X Up", view::upDir == view::UpDir::NegXUp)) {
        view::setUpDir(view::UpDir::NegXUp, true);
        ImGui::SetItemDefaultFocus();
      }
      if (ImGui::Selectable("Y Up", view::upDir == view::UpDir::YUp)) {
        view::setUpDir(view::UpDir::YUp, true);
        ImGui::SetItemDefaultFocus();
      }
      if (ImGui::Selectable("-Y Up", view::upDir == view::UpDir::NegYUp)) {
        view::setUpDir(view::UpDir::NegYUp, true);
        ImGui::SetItemDefaultFocus();
      }
      if (ImGui::Selectable("Z Up", view::upDir == view::UpDir::ZUp)) {
        view::setUpDir(view::UpDir::ZUp, true);
        ImGui::SetItemDefaultFocus();
      }
      if (ImGui::Selectable("-Z Up", view::upDir == view::UpDir::NegZUp)) {
        view::setUpDir(view::UpDir::NegZUp, true);
        ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Text("Up Direction");

    // Field of view
    float fovF = fov;
    if (ImGui::SliderFloat(" Field of View", &fovF, 5.0, 160.0, "%.2f deg")) {
      fov = fovF;
      requestRedraw();
    };

    // Clip planes
    float nearClipRatioF = nearClipRatio;
    float farClipRatioF = farClipRatio;
    if (ImGui::SliderFloat(" Clip Near", &nearClipRatioF, 0., 10., "%.5f", 3.)) {
      nearClipRatio = nearClipRatioF;
      requestRedraw();
    }
    if (ImGui::SliderFloat(" Clip Far", &farClipRatioF, 1., 1000., "%.2f", 3.)) {
      farClipRatio = farClipRatioF;
      requestRedraw();
    }

    // Move speed
    float moveScaleF = view::moveScale;
    ImGui::SliderFloat(" Move Speed", &moveScaleF, 0.0, 1.0, "%.5f", 3.);
    view::moveScale = moveScaleF;

    ImGui::PopItemWidth();


    ImGui::TreePop();
  }
}

void setUpDir(UpDir newUpDir, bool animateFlight) {
  upDir = newUpDir;
  if (animateFlight) {
    flyToHomeView();
  } else {
    resetCameraToHomeView();
  }
}

UpDir getUpDir() { return upDir; }

} // namespace view
} // namespace polyscope
