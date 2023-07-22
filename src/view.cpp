// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

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
FrontDir frontDir = FrontDir::ZFront;
double moveScale = 1.0;
const double defaultNearClipRatio = 0.005;
const double defaultFarClipRatio = 20.0;
const double defaultFov = 45.;
const double minFov = 5.;
const double maxFov = 160.;
double fov = defaultFov;
double nearClipRatio = defaultNearClipRatio;
double farClipRatio = defaultFarClipRatio;
ProjectionMode projectionMode = ProjectionMode::Perspective;
std::array<float, 4> bgColor{{1.0, 1.0, 1.0, 0.0}};

glm::mat4x4 viewMat;

bool midflight = false;
float flightStartTime = -1;
float flightEndTime = -1;
glm::dualquat flightTargetViewR, flightInitialViewR;
glm::vec3 flightTargetViewT, flightInitialViewT;
float flightTargetFov, flightInitialFov;


// Small helpers
std::string to_string(ProjectionMode mode) {
  switch (mode) {
  case ProjectionMode::Perspective:
    return "Perspective";
  case ProjectionMode::Orthographic:
    return "Orthographic";
  }
  return ""; // unreachable
}


void processRotate(glm::vec2 startP, glm::vec2 endP) {

  if (startP == endP) {
    return;
  }

  // Get frame
  glm::vec3 frameLookDir, frameUpDir, frameRightDir;
  getCameraFrame(frameLookDir, frameUpDir, frameRightDir);

  switch (getNavigateStyle()) {
  case NavigateStyle::Turntable: {

    glm::vec2 dragDelta = endP - startP;
    float delTheta = 2.0 * dragDelta.x * moveScale;
    float delPhi = 2.0 * dragDelta.y * moveScale;

    // Translate to center
    viewMat = glm::translate(viewMat, state::center());

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
    viewMat = glm::translate(viewMat, -state::center());
    break;
  }
  case NavigateStyle::Free: {
    glm::vec2 dragDelta = endP - startP;
    float delTheta = 2.0 * dragDelta.x * moveScale;
    float delPhi = 2.0 * dragDelta.y * moveScale;

    // Translate to center
    viewMat = glm::translate(viewMat, state::center());

    // Rotation about the vertical axis
    glm::mat4x4 thetaCamR = glm::rotate(glm::mat4x4(1.0), delTheta, frameUpDir);
    viewMat = viewMat * thetaCamR;

    // Rotation about the horizontal axis
    glm::mat4x4 phiCamR = glm::rotate(glm::mat4x4(1.0), -delPhi, frameRightDir);
    viewMat = viewMat * phiCamR;

    // Undo centering
    viewMat = glm::translate(viewMat, -state::center());
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

  switch (projectionMode) {
  case ProjectionMode::Perspective: {
    float movementScale = state::lengthScale * 0.1 * moveScale;
    glm::mat4x4 camSpaceT = glm::translate(glm::mat4x4(1.0), glm::vec3(0., 0., movementScale * amount));
    viewMat = camSpaceT * viewMat;
    break;
  }
  case ProjectionMode::Orthographic: {
    double fovScale = std::min(fov - minFov, maxFov - fov) / (maxFov - minFov);
    fov += -fovScale * amount;
    fov = glm::clamp(fov, minFov, maxFov);
    break;
  }
  }


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

  glm::vec3 target = state::center();
  glm::vec3 upDir = getUpVec();
  glm::vec3 frontDir = getFrontVec();
  if (std::fabs(glm::dot(upDir, frontDir)) > 0.01) {
    // if the user has foolishly set upDir and frontDir to be along the same axis,
    // change front dir so lookAt can do something sane
    frontDir = circularPermuteEntries(frontDir);
  }
  float L = state::lengthScale;
  glm::vec3 cameraLoc = state::center() + 0.1f * L * upDir + 1.5f * L * frontDir;
  return glm::lookAt(cameraLoc, target, upDir);
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


void lookAt(glm::vec3 cameraLocation, glm::vec3 target, bool flyTo) {
  lookAt(cameraLocation, target, getUpVec(), flyTo);
}

void lookAt(glm::vec3 cameraLocation, glm::vec3 target, glm::vec3 upDir, bool flyTo) {
  immediatelyEndFlight();
  glm::mat4x4 targetView = glm::lookAt(cameraLocation, target, upDir);

  // Give a sane warning for invalid inputs
  bool isFinite = true;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (!std::isfinite(targetView[i][j])) {
        isFinite = false;
      }
    }
  }
  if (!isFinite) {
    warning("lookAt() yielded an invalid view. Is the look direction collinear with the up direction?");
    // just continue after; our view handling will take care of the NaN and set it to the default view
  }

  if (flyTo) {
    startFlightTo(targetView, fov);
  } else {
    viewMat = targetView;
    requestRedraw();
  }
}

void setWindowSize(int width, int height) {
  view::windowWidth = width;
  view::windowHeight = height;
  render::engine->applyWindowSize();
}

std::tuple<int, int> getWindowSize() { return std::tuple<int, int>(view::windowWidth, view::windowHeight); }

std::tuple<int, int> getBufferSize() { return std::tuple<int, int>(view::bufferWidth, view::bufferWidth); }

void setViewToCamera(const CameraParameters& p) {
  viewMat = p.getE();
  fov = p.getFoVVerticalDegrees();
  // aspectRatio = p.focalLengths.x / p.focalLengths.y; // TODO should be
  // flipped?
}

CameraParameters getCameraParametersForCurrentView() {
  ensureViewValid();

  double aspectRatio = (float)bufferWidth / bufferHeight;
  return CameraParameters(CameraIntrinsics::fromFoVDegVerticalAndAspect(fov, aspectRatio),
                          CameraExtrinsics::fromMatrix(viewMat));
}

glm::mat4 getCameraViewMatrix() { return viewMat; }

glm::mat4 getCameraPerspectiveMatrix() {
  double farClip = farClipRatio * state::lengthScale;
  double nearClip = nearClipRatio * state::lengthScale;
  double fovRad = glm::radians(fov);
  double aspectRatio = (float)bufferWidth / bufferHeight;
  switch (projectionMode) {
  case ProjectionMode::Perspective: {
    return glm::perspective(fovRad, aspectRatio, nearClip, farClip);
    break;
  }
  case ProjectionMode::Orthographic: {
    double vert = tan(fovRad / 2.) * state::lengthScale * 2.;
    double horiz = vert * aspectRatio;
    return glm::ortho(-horiz, horiz, -vert, vert, nearClip, farClip);
    break;
  }
  }
  return glm::mat4(1.0f); // unreachable
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

glm::vec3 bufferCoordsToWorldRay(glm::vec2 screenCoords) {

  glm::mat4 view = getCameraViewMatrix();
  glm::mat4 proj = getCameraPerspectiveMatrix();
  glm::vec4 viewport = {0., 0., view::bufferWidth, view::bufferHeight};

  glm::vec3 screenPos3{screenCoords.x, view::bufferHeight - screenCoords.y, 0.};
  glm::vec3 worldPos = glm::unProject(screenPos3, view, proj, viewport);
  glm::vec3 worldRayDir = glm::normalize(glm::vec3(worldPos) - getCameraWorldPosition());

  return worldRayDir;
}


glm::vec3 screenCoordsToWorldPosition(glm::vec2 screenCoords) {

  glm::vec2 bufferCoords{screenCoords.x * static_cast<float>(view::bufferWidth) / static_cast<float>(view::windowWidth),
                         screenCoords.y * static_cast<float>(view::bufferHeight) /
                             static_cast<float>(view::windowHeight)};

  glm::mat4 view = getCameraViewMatrix();
  glm::mat4 viewInv = glm::inverse(view);
  glm::mat4 proj = getCameraPerspectiveMatrix();
  glm::mat4 projInv = glm::inverse(proj);
  // glm::vec2 depthRange = {0., 1.}; // no support for nonstandard depth range, currently

  // query the depth buffer to get depth
  render::FrameBuffer* sceneFramebuffer = render::engine->sceneBuffer.get();
  float depth = sceneFramebuffer->readDepth(bufferCoords.x, view::bufferHeight - bufferCoords.y);
  if (depth == 1.) {
    // if we didn't hit anything in the depth buffer, just return infinity
    float inf = std::numeric_limits<float>::infinity();
    return glm::vec3{inf, inf, inf};
  }

  // convert depth to world units
  glm::vec2 screenPos{screenCoords.x / static_cast<float>(view::windowWidth),
                      1.f - screenCoords.y / static_cast<float>(view::windowHeight)};
  float z = depth * 2.0f - 1.0f;
  glm::vec4 clipPos = glm::vec4(screenPos * 2.0f - 1.0f, z, 1.0f);
  glm::vec4 viewPos = projInv * clipPos;
  viewPos /= viewPos.w;

  glm::vec4 worldPos = viewInv * viewPos;
  worldPos /= worldPos.w;

  return glm::vec3(worldPos);
}

void startFlightTo(const CameraParameters& p, float flightLengthInSeconds) {
  startFlightTo(p.getE(), p.getFoVVerticalDegrees(), flightLengthInSeconds);
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

std::string getViewAsJson() {

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
      {"windowWidth", view::windowWidth},
      {"windowHeight", view::windowHeight},
      {"projectionMode", to_string(view::projectionMode)},
  };

  std::string outString = j.dump();
  return outString;
}
std::string getCameraJson() { return getViewAsJson(); }

void setViewFromJson(std::string jsonData, bool flyTo) {
  // Values will go here
  glm::mat4 newViewMat;
  double newFov = -777;
  double newNearClipRatio = -777;
  double newFarClipRatio = -777;

  int windowWidth = view::windowWidth;
  int windowHeight = view::windowHeight;

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

    // Get the window sizes, if present
    if (j.find("windowWidth") != j.end()) {
      windowWidth = j["windowWidth"];
    }
    if (j.find("windowHeight") != j.end()) {
      windowHeight = j["windowHeight"];
    }

    if (j.find("projectionMode") != j.end()) {
      std::string projectionModeStr = j["projectionMode"];
      if (projectionModeStr == to_string(ProjectionMode::Perspective)) {
        view::projectionMode = ProjectionMode::Perspective;
      } else if (projectionModeStr == to_string(ProjectionMode::Orthographic)) {
        view::projectionMode = ProjectionMode::Orthographic;
      }
    }

  } catch (...) {
    // If anything goes wrong parsing, just give up
    return;
  }

  // === Assign the new values

  view::setWindowSize(windowWidth, windowHeight);

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
void setCameraFromJson(std::string jsonData, bool flyTo) { setViewFromJson(jsonData, flyTo); }

void buildViewGui() {

  ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
  if (openSlicePlaneMenu) {
    // need to recursively open this tree node to respect slice plane menu open flag
    ImGui::SetNextTreeNodeOpen(true);
  }
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
        setNavigateStyle(view::NavigateStyle::Turntable, true);
        ImGui::SetItemDefaultFocus();
      }
      if (ImGui::Selectable("Free", view::style == view::NavigateStyle::Free)) {
        setNavigateStyle(view::NavigateStyle::Free, true);
        ImGui::SetItemDefaultFocus();
      }
      if (ImGui::Selectable("Planar", view::style == view::NavigateStyle::Planar)) {
        setNavigateStyle(view::NavigateStyle::Planar, true);
        ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::SameLine();

    ImGui::Text("Camera Style");

    { // == Up direction
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
    }

    { // == Front direction
      ImGui::PushItemWidth(120);
      std::string frontStyleName;
      switch (frontDir) {
      case FrontDir::XFront:
        frontStyleName = "X Front";
        break;
      case FrontDir::NegXFront:
        frontStyleName = "-X Front";
        break;
      case FrontDir::YFront:
        frontStyleName = "Y Front";
        break;
      case FrontDir::NegYFront:
        frontStyleName = "-Y Front";
        break;
      case FrontDir::ZFront:
        frontStyleName = "Z Front";
        break;
      case FrontDir::NegZFront:
        frontStyleName = "-Z Front";
        break;
      }

      if (ImGui::BeginCombo("##Front Direction", frontStyleName.c_str())) {
        if (ImGui::Selectable("X Front", view::frontDir == FrontDir::XFront)) {
          view::setFrontDir(FrontDir::XFront, true);
          ImGui::SetItemDefaultFocus();
        }
        if (ImGui::Selectable("-X Front", view::frontDir == FrontDir::NegXFront)) {
          view::setFrontDir(FrontDir::NegXFront, true);
          ImGui::SetItemDefaultFocus();
        }
        if (ImGui::Selectable("Y Front", view::frontDir == FrontDir::YFront)) {
          view::setFrontDir(FrontDir::YFront, true);
          ImGui::SetItemDefaultFocus();
        }
        if (ImGui::Selectable("-Y Front", view::frontDir == FrontDir::NegYFront)) {
          view::setFrontDir(FrontDir::NegYFront, true);
          ImGui::SetItemDefaultFocus();
        }
        if (ImGui::Selectable("Z Front", view::frontDir == FrontDir::ZFront)) {
          view::setFrontDir(FrontDir::ZFront, true);
          ImGui::SetItemDefaultFocus();
        }
        if (ImGui::Selectable("-Z Front", view::frontDir == FrontDir::NegZFront)) {
          view::setFrontDir(FrontDir::NegZFront, true);
          ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::SameLine();
      ImGui::Text("Front Direction");
    }

    {
      // Show a warning if up and front are co-linear
      glm::vec3 upDir = getUpVec();
      glm::vec3 frontDir = getFrontVec();
      if (std::fabs(glm::dot(upDir, frontDir)) > 0.01) {
        ImGui::TextUnformatted("WARNING: Up and Front directions\nare degenerate.");
      }
    }

    if (ImGui::TreeNode("Scene Extents")) {

      if (ImGui::Checkbox("Set automatically", &options::automaticallyComputeSceneExtents)) {
        updateStructureExtents();
      }

      if (!options::automaticallyComputeSceneExtents) {

        static float lengthScaleUpper = -777;
        if (lengthScaleUpper == -777) lengthScaleUpper = 2. * state::lengthScale;
        if (ImGui::SliderFloat("Length Scale", &state::lengthScale, 0, lengthScaleUpper, "%.5f")) {
          requestRedraw();
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
          // the upper bound for the slider is dynamically adjust to be a bit bigger than the lower bound, but only does
          // so on release of the widget (so it doesn't scaleo off to infinity), and only ever gets larger (so you don't
          // get stuck at 0)
          lengthScaleUpper = std::fmax(2. * state::lengthScale, lengthScaleUpper);
        }


        ImGui::TextUnformatted("Bounding Box:");
        ImGui::PushItemWidth(200);
        glm::vec3& bboxMin = std::get<0>(state::boundingBox);
        glm::vec3& bboxMax = std::get<1>(state::boundingBox);
        if (ImGui::InputFloat3("min", &bboxMin[0])) updateStructureExtents();
        if (ImGui::InputFloat3("max", &bboxMax[0])) updateStructureExtents();
        ImGui::PopItemWidth();
      }


      ImGui::TreePop();
    }


    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Camera Parameters")) {

      // Field of view
      float fovF = fov;
      if (ImGui::SliderFloat(" Field of View", &fovF, minFov, maxFov, "%.2f deg")) {
        fov = fovF;
        requestRedraw();
      };

      // Clip planes
      float nearClipRatioF = nearClipRatio;
      float farClipRatioF = farClipRatio;
      if (ImGui::SliderFloat(" Clip Near", &nearClipRatioF, 0., 10., "%.5f",
                             ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
        nearClipRatio = nearClipRatioF;
        requestRedraw();
      }
      if (ImGui::SliderFloat(" Clip Far", &farClipRatioF, 1., 1000., "%.2f",
                             ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
        farClipRatio = farClipRatioF;
        requestRedraw();
      }

      // Move speed
      float moveScaleF = view::moveScale;
      ImGui::SliderFloat(" Move Speed", &moveScaleF, 0.0, 1.0, "%.5f",
                         ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
      view::moveScale = moveScaleF;


      std::string projectionModeStr = to_string(view::projectionMode);
      if (ImGui::BeginCombo("##ProjectionMode", projectionModeStr.c_str())) {
        if (ImGui::Selectable("Perspective", view::projectionMode == ProjectionMode::Perspective)) {
          view::projectionMode = ProjectionMode::Perspective;
          requestRedraw();
          ImGui::SetItemDefaultFocus();
        }
        if (ImGui::Selectable("Orthographic", view::projectionMode == ProjectionMode::Orthographic)) {
          view::projectionMode = ProjectionMode::Orthographic;
          requestRedraw();
          ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::SameLine();
      ImGui::Text("Projection");


      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Window")) {

      {
        ImGui::TextUnformatted("Dim:");
        ImGui::SameLine();
        ImGui::PushItemWidth(50);
        bool changed = false;
        int currWidth = view::windowWidth;
        int currHeight = view::windowHeight;
        ImGui::InputInt("##width", &currWidth, 0);
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        ImGui::SameLine();
        ImGui::InputInt("##height", &currHeight, 0);
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        if (changed) {
          // make sure it's at least 32 pixels, anything less is surely a mistake and might break things
          currWidth = std::max(currWidth, 32);
          currHeight = std::max(currHeight, 32);
          view::setWindowSize(currWidth, currHeight);
        }
        ImGui::PopItemWidth();
      }

      {
        ImGui::SameLine();
        bool sizeLocked = !getWindowResizable();
        bool changed = ImGui::Checkbox("lock", &sizeLocked);
        if (changed) {
          setWindowResizable(!sizeLocked);
        }
      }


      ImGui::TreePop();
    }

    buildSlicePlaneGUI();

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

glm::vec3 getUpVec() {
  switch (upDir) {
  case UpDir::NegXUp:
    return glm::vec3{-1., 0., 0.};
  case UpDir::XUp:
    return glm::vec3{1., 0., 0.};
  case UpDir::NegYUp:
    return glm::vec3{0., -1., 0.};
  case UpDir::YUp:
    return glm::vec3{0., 1., 0.};
  case UpDir::NegZUp:
    return glm::vec3{0., 0., -1.};
  case UpDir::ZUp:
    return glm::vec3{0., 0., 1.};
  }

  // unused fallthrough
  return glm::vec3{0., 0., 0.};
}

void setFrontDir(FrontDir newFrontDir, bool animateFlight) {
  frontDir = newFrontDir;
  if (animateFlight) {
    flyToHomeView();
  } else {
    resetCameraToHomeView();
  }
  requestRedraw();
}

FrontDir getFrontDir() { return frontDir; }

glm::vec3 getFrontVec() {
  switch (frontDir) {
  case FrontDir::NegXFront:
    return glm::vec3{-1., 0., 0.};
  case FrontDir::XFront:
    return glm::vec3{1., 0., 0.};
  case FrontDir::NegYFront:
    return glm::vec3{0., -1., 0.};
  case FrontDir::YFront:
    return glm::vec3{0., 1., 0.};
  case FrontDir::NegZFront:
    return glm::vec3{0., 0., -1.};
  case FrontDir::ZFront:
    return glm::vec3{0., 0., 1.};
  }

  // unused fallthrough
  return glm::vec3{0., 0., 0.};
}


void setNavigateStyle(NavigateStyle newNavigateStyle, bool animateFlight) {
  style = newNavigateStyle;
  if (animateFlight) {
    flyToHomeView();
  } else {
    resetCameraToHomeView();
  }
}
NavigateStyle getNavigateStyle() { return style; }

void setWindowResizable(bool isResizable) { return render::engine->setWindowResizable(isResizable); }

bool getWindowResizable() { return render::engine->getWindowResizable(); }


} // namespace view
} // namespace polyscope
