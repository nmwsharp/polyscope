// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/view.h"

#include "polyscope/polyscope.h"
#include "polyscope/utilities.h"

#include "imgui.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace polyscope {
namespace view {


// Storage for state variables
int& bufferWidth = state::globalContext.bufferWidth;
int& bufferHeight = state::globalContext.bufferHeight;
int& windowWidth = state::globalContext.windowWidth;
int& windowHeight = state::globalContext.windowHeight;
int& initWindowPosX = state::globalContext.initWindowPosX;
int& initWindowPosY = state::globalContext.initWindowPosY;
bool& windowResizable = state::globalContext.windowResizable;
NavigateStyle& style = state::globalContext.navigateStyle;
UpDir& upDir = state::globalContext.upDir;
FrontDir& frontDir = state::globalContext.frontDir;
float& moveScale = state::globalContext.moveScale;
ViewRelativeMode& viewRelativeMode = state::globalContext.viewRelativeMode;
float& nearClip = state::globalContext.nearClip;
float& farClip = state::globalContext.farClip;
std::array<float, 4>& bgColor = state::globalContext.bgColor;
glm::mat4x4& viewMat = state::globalContext.viewMat;
float& fov = state::globalContext.fov;
ProjectionMode& projectionMode = state::globalContext.projectionMode;
glm::vec3& viewCenter = state::globalContext.viewCenter;
bool& midflight = state::globalContext.midflight;
float& flightStartTime = state::globalContext.flightStartTime;
float& flightEndTime = state::globalContext.flightEndTime;
glm::dualquat& flightTargetViewR = state::globalContext.flightTargetViewR;
glm::dualquat& flightInitialViewR = state::globalContext.flightInitialViewR;
glm::vec3& flightTargetViewT = state::globalContext.flightTargetViewT;
glm::vec3& flightInitialViewT = state::globalContext.flightInitialViewT;
float& flightTargetFov = state::globalContext.flightTargetFov;
float& flightInitialFov = state::globalContext.flightInitialFov;


// Default values
const int defaultWindowWidth = 1280;
const int defaultWindowHeight = 720;
const float defaultNearClipRatio = 0.005f;
const float defaultFarClipRatio = 20.f;
const float defaultFov = 45.;
const float minFov = 5.;   // for UI
const float maxFov = 160.; // for UI

// Internal details
bool overrideClipPlanes =
    false; // used only for temporary state changes in render passes, so we do not track in context
float overrideNearClipRelative = defaultNearClipRatio;
float overrideFarClipRelative = defaultFarClipRatio;

// Small helpers

namespace { // anonymous helpers

// A default pairing of <up,front> directions to fall back on when something goes wrong.
const std::vector<std::pair<UpDir, FrontDir>> defaultUpFrontPairs{
    {UpDir::NegXUp, FrontDir::NegYFront}, {UpDir::XUp, FrontDir::YFront},       {UpDir::NegYUp, FrontDir::NegZFront},
    {UpDir::YUp, FrontDir::ZFront},       {UpDir::NegZUp, FrontDir::NegXFront}, {UpDir::ZUp, FrontDir::XFront}};

FrontDir defaultOrthogonalFrontDir(UpDir upDir) {
  for (const std::pair<UpDir, FrontDir>& p : defaultUpFrontPairs) {
    if (p.first == upDir) return p.second;
  }
  return FrontDir::ZFront; // fallthrough, should be unused
}

UpDir defaultOrthogonalUpDir(FrontDir frontDir) {
  for (const std::pair<UpDir, FrontDir>& p : defaultUpFrontPairs) {
    if (p.second == frontDir) return p.first;
  }
  return UpDir::YUp; // fallthrough, should be unused
}

}; // namespace


std::tuple<int, int> screenCoordsToBufferInds(glm::vec2 screenCoords) {

  int xPos = (screenCoords.x * view::bufferWidth) / view::windowWidth;
  int yPos = (screenCoords.y * view::bufferHeight) / view::windowHeight;

  // clamp to lie in [0,width),[0,height)
  xPos = std::max(std::min(xPos, view::bufferWidth - 1), 0);
  yPos = std::max(std::min(yPos, view::bufferHeight - 1), 0);

  return std::tuple<int, int>(xPos, yPos);
}

glm::ivec2 screenCoordsToBufferIndsVec(glm::vec2 screenCoords) {
  glm::ivec2 out;
  std::tie(out.x, out.y) = screenCoordsToBufferInds(screenCoords);
  return out;
}

glm::vec2 bufferIndsToScreenCoords(int xPos, int yPos) {
  return glm::vec2{xPos * static_cast<float>(view::windowWidth) / view::bufferWidth,
                   yPos * static_cast<float>(view::windowHeight) / view::bufferHeight};
}

glm::vec2 bufferIndsToScreenCoords(glm::ivec2 bufferInds) {
  return bufferIndsToScreenCoords(bufferInds.x, bufferInds.y);
}

std::string getCurrentProjectionModeRaycastRule() {
  // This is related to the render engine, it returns the name of the
  // shader rule which constructs rays for raycasting-based rendering.
  // See rules.h

  switch (view::projectionMode) {
  case ProjectionMode::Perspective:
    return "BUILD_RAY_FOR_FRAGMENT_PERSPECTIVE";
  case ProjectionMode::Orthographic:
    return "BUILD_RAY_FOR_FRAGMENT_ORTHOGRAPHIC";
  }
  return "";
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

    // Disallow rotations that would almost align the vertical axis
    glm::vec3 verticalAxis = getUpVec();
    float lim = 0.01f; // controls how close to vertical the view can get
    if (glm::dot(frameLookDir, verticalAxis) > 1.0 - lim) {
      delPhi = std::min(delPhi, 0.0f);
    }
    if (glm::dot(frameLookDir, verticalAxis) < -1.0 + lim) {
      delPhi = std::max(delPhi, 0.0f);
    }

    // Translate to center
    viewMat = glm::translate(viewMat, view::viewCenter);

    // Rotation about the horizontal axis
    glm::mat4x4 phiCamR = glm::rotate(glm::mat4x4(1.0), -delPhi, frameRightDir);
    viewMat = viewMat * phiCamR;

    // Rotation about the vertical axis
    glm::vec3 turntableUp;
    glm::mat4x4 thetaCamR = glm::rotate(glm::mat4x4(1.0), delTheta, getUpVec());
    viewMat = viewMat * thetaCamR;

    // Undo centering
    viewMat = glm::translate(viewMat, -view::viewCenter);

    // Enforce that the view indeed looks towards the center, as it always should with Turntable mode.
    // Mostly this will have no effect, but it can prevent gradual numerical drift where the center shifts relvative to
    // the view matrix.
    lookAt(view::getCameraWorldPosition(), view::viewCenter, view::getUpVec(), false);

    break;
  }
  case NavigateStyle::Free: {
    glm::vec2 dragDelta = endP - startP;
    float delTheta = 2.0 * dragDelta.x * moveScale;
    float delPhi = 2.0 * dragDelta.y * moveScale;

    // Translate to center
    viewMat = glm::translate(viewMat, view::viewCenter);

    // Rotation about the vertical axis
    glm::mat4x4 thetaCamR = glm::rotate(glm::mat4x4(1.0), delTheta, frameUpDir);
    viewMat = viewMat * thetaCamR;

    // Rotation about the horizontal axis
    glm::mat4x4 phiCamR = glm::rotate(glm::mat4x4(1.0), -delPhi, frameRightDir);
    viewMat = viewMat * phiCamR;

    // Undo centering
    viewMat = glm::translate(viewMat, -view::viewCenter);
    break;
  }
  case NavigateStyle::Planar: {
    // Do nothing
    break;
  }
  case NavigateStyle::Arcball: {
    // Map inputs to unit sphere
    auto toSphere = [](glm::vec2 v) {
      float x = glm::clamp(v.x, -1.0f, 1.0f);
      float y = glm::clamp(v.y, -1.0f, 1.0f);
      float mag = x * x + y * y;
      if (mag <= 1.0) {
        return glm::vec3{x, y, -std::sqrt(1.0 - mag)};
      } else {
        return glm::normalize(glm::vec3{x, y, 0.0});
      }
    };
    glm::vec3 sphereStart = toSphere(startP);
    glm::vec3 sphereEnd = toSphere(endP);

    glm::vec3 rotAxis = -cross(sphereStart, sphereEnd);
    float rotMag = std::acos(glm::clamp(dot(sphereStart, sphereEnd), -1.0f, 1.0f) * moveScale);

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
  case NavigateStyle::None: {
    // Do nothing
    break;
  }
  case NavigateStyle::FirstPerson: {
    glm::vec2 dragDelta = endP - startP;
    float delTheta = 2.0 * dragDelta.x;
    float delPhi = 2.0 * dragDelta.y;

    // Rotation about the vertical axis
    glm::vec3 rotAx = glm::mat3(viewMat) * getUpVec();
    glm::mat4x4 thetaCamR = glm::rotate(glm::mat4x4(1.0), delTheta, rotAx);
    viewMat = thetaCamR * viewMat;

    // Rotation about the horizontal axis
    glm::mat4x4 phiCamR = glm::rotate(glm::mat4x4(1.0), -delPhi, glm::vec3(1.f, 0.f, 0.f));
    viewMat = phiCamR * viewMat;

    break;
  }
  }

  requestRedraw();
  immediatelyEndFlight();
}

void processTranslate(glm::vec2 delta) {
  if (getNavigateStyle() == NavigateStyle::None) {
    return;
  }
  if (glm::length(delta) == 0) {
    return;
  }

  // Process a translation
  float s = computeRelativeMotionScale();
  float movementScale = 0.6f * s * moveScale;
  glm::mat4x4 camSpaceT = glm::translate(glm::mat4x4(1.0), movementScale * glm::vec3(delta.x, delta.y, 0.0));
  viewMat = camSpaceT * viewMat;

  if (getNavigateStyle() == NavigateStyle::Turntable) {
    // also translate the turntable center according to the same motion
    glm::vec3 oldCenter = view::viewCenter;
    glm::vec3 worldspaceT =
        glm::transpose(glm::mat3(viewMat)) * glm::vec3(-movementScale * delta.x, -movementScale * delta.y, 0.0);
    glm::vec3 newCenter = oldCenter + worldspaceT;
    setViewCenterRaw(newCenter);
  }

  projectCenterToBeValidForView();

  requestRedraw();
  immediatelyEndFlight();
}

void processClipPlaneShift(float amount) {
  if (amount == 0.0) return;
  // Adjust the near clipping plane
  nearClip += .03 * amount * nearClip;
  requestRedraw();
}

void processZoom(float amount) {
  if (amount == 0.0) return;
  if (getNavigateStyle() == NavigateStyle::None || getNavigateStyle() == NavigateStyle::FirstPerson) {
    return;
  }

  // Translate the camera forwards and backwards

  switch (projectionMode) {
  case ProjectionMode::Perspective: {
    float s = computeRelativeMotionScale();
    float totalZoom = 0.15f * s * amount;

    // Disallow zooming that would cross the center point
    if (getNavigateStyle() == NavigateStyle::Turntable) {
      float currSignedDistToCenter = glm::dot(getLookVec(), view::viewCenter - view::getCameraWorldPosition());
      float minDistToCenter = computeRelativeMotionScale() * 1e-5;
      float maxAllowedZoom = currSignedDistToCenter - minDistToCenter;
      totalZoom = glm::min(totalZoom, maxAllowedZoom);
    }

    glm::mat4x4 camSpaceT = glm::translate(glm::mat4x4(1.0), glm::vec3(0., 0., totalZoom));
    viewMat = camSpaceT * viewMat;

    break;
  }
  case ProjectionMode::Orthographic: {
    float fovScale = std::min(fov - minFov, maxFov - fov) / (maxFov - minFov);
    fov += -fovScale * amount;
    fov = glm::clamp(fov, minFov, maxFov);
    break;
  }
  }


  immediatelyEndFlight();
  requestRedraw();
}

void processKeyboardNavigation(ImGuiIO& io) {


  // == Non movement-related

  // ctrl-c
  if (io.KeyCtrl && render::engine->isKeyPressed('c')) {
    std::string outData = view::getCameraJson();
    render::engine->setClipboardText(outData);
  }

  // ctrl-v
  if (io.KeyCtrl && render::engine->isKeyPressed('v')) {
    std::string clipboardData = render::engine->getClipboardText();
    view::setCameraFromJson(clipboardData, true);
  }


  // == Movement-related
  bool hasMovement = false;

  if (getNavigateStyle() == NavigateStyle::FirstPerson) {

    // WASD-style keyboard navigation

    glm::vec3 delta{0.f, 0.f, 0.f};

    if (ImGui::IsKeyDown(ImGuiKey_A)) delta.x += 1.f;
    if (ImGui::IsKeyDown(ImGuiKey_D)) delta.x += -1.f;
    if (ImGui::IsKeyDown(ImGuiKey_Q)) delta.y += 1.f;
    if (ImGui::IsKeyDown(ImGuiKey_E)) delta.y += -1.f;
    if (ImGui::IsKeyDown(ImGuiKey_W)) delta.z += 1.f;
    if (ImGui::IsKeyDown(ImGuiKey_S)) delta.z += -1.f;

    if (glm::length(delta) > 0.) {
      hasMovement = true;
    }

    float s = computeRelativeMotionScale();
    float movementMult = s * ImGui::GetIO().DeltaTime * moveScale;
    glm::mat4x4 camSpaceT = glm::translate(glm::mat4x4(1.0), movementMult * delta);
    viewMat = camSpaceT * viewMat;
  }

  if (hasMovement) {
    immediatelyEndFlight();
    requestRedraw();
  }
}

void processSetCenter(glm::vec2 screenCoords) {
  PickResult pickResult = pickAtScreenCoords(screenCoords);

  if (pickResult.isHit) {
    setViewCenterAndLookAt(pickResult.position, true);
  }
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

float computeRelativeMotionScale() {
  switch (viewRelativeMode) {
  case ViewRelativeMode::CenterRelative: {
    float distToCenter = glm::length(view::viewCenter - view::getCameraWorldPosition());
    return distToCenter;
  }
  case ViewRelativeMode::LengthRelative: {
    return state::lengthScale;
  }
  }
  return -1.; // should be unreachable
}

void projectCenterToBeValidForView() {
  // If necessary, move the view center to be one that is compatible with the current view matrix and navigation style.

  switch (style) {
  case NavigateStyle::Turntable: {

    // Center must lie exactly along the camera look direction

    glm::vec3 camPos = getCameraWorldPosition();
    glm::vec3 camLookDir, camUpDir, camRightDir;
    getCameraFrame(camLookDir, camUpDir, camRightDir);
    glm::vec3 sceneUpDir = getUpVec();
    glm::vec3 vecToCenter = viewCenter - camPos;
    float distToCenter = glm::length(vecToCenter);
    glm::vec3 newCenter = camPos + camLookDir * distToCenter;

    setViewCenterRaw(newCenter);

    break;
  }
  case NavigateStyle::Planar:
  case NavigateStyle::Free:
  case NavigateStyle::Arcball:
  case NavigateStyle::None:
  case NavigateStyle::FirstPerson:
    // No constraints
    break;
  }
}

glm::mat4 computeHomeView() {

  glm::vec3 target = view::viewCenter;
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

  view::viewCenter = state::center();
  viewMat = computeHomeView();

  fov = defaultFov;
  nearClip = defaultNearClipRatio;
  farClip = defaultFarClipRatio;

  requestRedraw();
}

void flyToHomeView() {

  // WARNING: Duplicated here and in resetCameraToHomeView()

  view::viewCenter = state::center();
  glm::mat4x4 T = computeHomeView();

  float Tfov = defaultFov;
  nearClip = defaultNearClipRatio;
  farClip = defaultFarClipRatio;

  startFlightTo(T, Tfov);
}

void updateViewAndChangeNavigationStyle(NavigateStyle newStyle, bool flyTo) {
  NavigateStyle oldStyle = view::style;
  view::style = newStyle;

  if (viewIsValid()) {
    // for a few combinations of views, we can leave the camera where it is rather than resetting to the home view
    if (newStyle == NavigateStyle::Free) {
      // nothing needed
    } else if (newStyle == NavigateStyle::FirstPerson && oldStyle == NavigateStyle::Turntable) {
      // nothing needed
    } else if (newStyle == NavigateStyle::Turntable) {
      // leave the camera in the same location
      lookAt(getCameraWorldPosition(), view::viewCenter, flyTo);
    } else {
      // General case, depending only on the target style
      glm::mat4x4 T = computeHomeView();
      if (flyTo) {
        startFlightTo(T, view::fov);
      } else {
        viewMat = T;
      }
    }

    requestRedraw();
  }
}

void updateViewAndChangeUpDir(UpDir newUpDir, bool flyTo) {
  view::upDir = newUpDir;

  if (std::fabs(dot(view::getUpVec(), view::getFrontVec())) > 0.1) {
    // if the user has foolishly set upDir and frontDir to be along the same axis, fix it
    view::frontDir = defaultOrthogonalFrontDir(view::upDir);
  }

  if (viewIsValid()) {
    switch (style) {
    case NavigateStyle::Turntable:
    case NavigateStyle::Planar:
    case NavigateStyle::Arcball:
    case NavigateStyle::FirstPerson: {
      glm::vec3 lookDir = getCameraParametersForCurrentView().getLookDir();
      if (std::fabs(dot(view::getUpVec(), lookDir)) < 0.01) {
        // if the new up direction is colinear with the direction we're currently looking
        lookDir = getFrontVec();
      }

      glm::vec3 camPos = getCameraWorldPosition();
      lookAt(camPos, camPos + lookDir * state::lengthScale, flyTo);

      break;
    }
    case NavigateStyle::Free:
    case NavigateStyle::None:
      // No change needed
      break;
    }

    requestRedraw();
  }
}

void updateViewAndChangeFrontDir(FrontDir newFrontDir, bool flyTo) {
  view::frontDir = newFrontDir;

  if (std::fabs(dot(view::getUpVec(), view::getFrontVec())) > 0.1) {
    // if the user has foolishly set upDir and frontDir to be along the same axis, fix it
    setUpDir(defaultOrthogonalUpDir(view::frontDir), flyTo);
  }

  if (viewIsValid()) {
    switch (style) {
    case NavigateStyle::Turntable:
    case NavigateStyle::Planar:
    case NavigateStyle::Arcball:
    case NavigateStyle::Free:
    case NavigateStyle::FirstPerson:
    case NavigateStyle::None:
      // Currently no views require updating to conform to the front dir, it is just for the default pose
      break;
    }

    requestRedraw();
  }
}

void setViewCenterAndLookAt(glm::vec3 newCenter, bool flyTo) {

  view::viewCenter = newCenter;

  if (viewIsValid()) {
    // Update the view to be relative to the new center
    // This is necessary for some view modes like Turntable, where the viewMat is in a constrained family with respect
    // to the center.
    switch (style) {
    case NavigateStyle::Turntable:
    case NavigateStyle::Arcball:
    case NavigateStyle::Free:
    case NavigateStyle::FirstPerson:
      // this is a decent baseliny policy that always does _something_ sane
      // might want nicer policies for certain cameras
      lookAt(getCameraWorldPosition(), view::viewCenter, flyTo);
      break;
    case NavigateStyle::Planar: {
      // move the camera within the planar constraint
      glm::vec3 lookDir = getCameraParametersForCurrentView().getLookDir();
      glm::vec3 camPos = getCameraWorldPosition();
      glm::vec3 targetVec = newCenter - camPos;
      glm::vec3 planarDir = getFrontVec();
      glm::vec3 newCamPos = newCenter - planarDir * glm::dot(planarDir, targetVec);
      lookAt(newCamPos, view::viewCenter, flyTo);
      break;
    }
    case NavigateStyle::None:
      // no change needed
      break;
    }

    requestRedraw();
  }
}

void setViewCenterAndProject(glm::vec3 newCenter) {
  view::viewCenter = newCenter;
  projectCenterToBeValidForView();
}

void setViewCenterRaw(glm::vec3 newCenter) { view::viewCenter = newCenter; }

glm::vec3 getViewCenter() { return view::viewCenter; }

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
    warning("lookAt() yielded an invalid view. Is the location same as the target? Is the look direction collinear "
            "with the up direction?");
    // just continue after; our view handling will take care of the NaN and set it to the default view
  }


  if (flyTo) {
    startFlightTo(targetView, fov);
  } else {
    viewMat = targetView;
    projectCenterToBeValidForView();
    requestRedraw();
  }
}

void setWindowSize(int width, int height) {
  view::windowWidth = width;
  view::windowHeight = height;
  if (isInitialized()) {
    render::engine->applyWindowSize();
  }
}

std::tuple<int, int> getWindowSize() { return std::tuple<int, int>(view::windowWidth, view::windowHeight); }

std::tuple<int, int> getBufferSize() { return std::tuple<int, int>(view::bufferWidth, view::bufferHeight); }

void setViewToCamera(const CameraParameters& p) {
  viewMat = p.getE();
  fov = p.getFoVVerticalDegrees();
  // aspectRatio = p.focalLengths.x / p.focalLengths.y; // TODO should be
  // flipped?
  projectCenterToBeValidForView();
}

CameraParameters getCameraParametersForCurrentView() {
  ensureViewValid();

  float aspectRatio = (float)bufferWidth / bufferHeight;
  return CameraParameters(CameraIntrinsics::fromFoVDegVerticalAndAspect(fov, aspectRatio),
                          CameraExtrinsics::fromMatrix(viewMat));
}

void setCameraViewMatrix(glm::mat4 mat) {
  viewMat = mat;
  projectCenterToBeValidForView();
  requestRedraw();
}

glm::mat4 getCameraViewMatrix() { return viewMat; }

void setVerticalFieldOfViewDegrees(float newVal) {
  view::fov = newVal;
  requestRedraw();
}

ProjectionMode getProjectionMode() { return projectionMode; }

void setProjectionMode(ProjectionMode newMode) {
  projectionMode = newMode;
  internal::lazy::projectionMode = newMode; // update the lazy property right now, so we don't pay for a refresh twice
  refresh();
  requestRedraw();
}


ViewRelativeMode getViewRelativeMode() { return viewRelativeMode; }
void setViewRelativeMode(ViewRelativeMode newMode) {
  viewRelativeMode = newMode;
  requestRedraw();
}
void setClipPlanes(float newNearClip, float newFarClip) {
  nearClip = newNearClip;
  farClip = newFarClip;
  requestRedraw();
}
std::tuple<float, float> getClipPlanes() { return std::tuple<float, float>(nearClip, farClip); }

float getVerticalFieldOfViewDegrees() { return view::fov; }

float getAspectRatioWidthOverHeight() { return (float)bufferWidth / bufferHeight; }

glm::mat4 getCameraPerspectiveMatrix() {

  // Set the clip plane
  float absNearClip, absFarClip;
  if (overrideClipPlanes) {
    absNearClip = overrideNearClipRelative * state::lengthScale;
    absFarClip = overrideFarClipRelative * state::lengthScale;
  } else {
    std::tie(absNearClip, absFarClip) = computeClipPlanes();
  }

  float fovRad = glm::radians(fov);
  float aspectRatio = (float)bufferWidth / bufferHeight;
  switch (projectionMode) {
  case ProjectionMode::Perspective: {
    return glm::perspective(fovRad, aspectRatio, absNearClip, absFarClip);
    break;
  }
  case ProjectionMode::Orthographic: {
    float vert = tan(fovRad / 2.) * state::lengthScale * 2.;
    float horiz = vert * aspectRatio;
    return glm::ortho(-horiz, horiz, -vert, vert, absNearClip, absFarClip);
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

glm::vec3 getLookVec() {
  glm::mat3x3 R;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      R[i][j] = viewMat[i][j];
    }
  }
  glm::mat3x3 Rt = glm::transpose(R);

  return Rt * glm::vec3(0.0, 0.0, -1.0);
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

glm::vec3 bufferIndsToWorldRay(glm::vec2 bufferInds) { return bufferCoordsToWorldRay(bufferInds); }

glm::vec3 bufferCoordsToWorldRay(glm::vec2 bufferCoords) {

  glm::mat4 view = getCameraViewMatrix();
  glm::mat4 proj = getCameraPerspectiveMatrix();
  glm::vec4 viewport = {0., 0., view::bufferWidth, view::bufferHeight};

  glm::vec3 screenPos3{bufferCoords.x, view::bufferHeight - bufferCoords.y, 0.};
  glm::vec3 worldPos = glm::unProject(screenPos3, view, proj, viewport);
  glm::vec3 worldRayDir = glm::normalize(glm::vec3(worldPos) - getCameraWorldPosition());

  return worldRayDir;
}

std::tuple<float, float> computeClipPlanes() {
  float s = computeRelativeMotionScale();
  float absFarClip = farClip * s;

  // it's tempting to compute both near and far clip planes from the scale,
  // but computing the near clip plane this way gives flickering very quickly when zooming in on surface details
  // float absNearClip = nearClip * s;

  // instead, always use the length scale for the near clip plane
  float absNearClip = nearClip * state::lengthScale;

  return std::make_tuple(absNearClip, absFarClip);
}

glm::vec3 screenCoordsAndDepthToWorldPosition(glm::vec2 screenCoords, float clipDepth) {

  if (clipDepth == 1.) {
    // if we didn't hit anything in the depth buffer, just return infinity
    float inf = std::numeric_limits<float>::infinity();
    return glm::vec3{inf, inf, inf};
  }


  glm::mat4 view = getCameraViewMatrix();
  glm::mat4 viewInv = glm::inverse(view);
  glm::mat4 proj = getCameraPerspectiveMatrix();
  glm::mat4 projInv = glm::inverse(proj);
  // glm::vec2 depthRange = {0., 1.}; // no support for nonstandard depth range, currently

  // convert depth to world units
  glm::vec2 screenPos{screenCoords.x / static_cast<float>(view::windowWidth),
                      1.f - screenCoords.y / static_cast<float>(view::windowHeight)};
  float z = clipDepth * 2.0f - 1.0f;
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

  // NOTE: we interpolate the _inverse_ view matrix (then invert back), because it looks better
  // when far from the origin

  // Initial parameters
  glm::mat3x4 Rstart;
  glm::vec3 Tstart;
  glm::mat4 viewInv = glm::inverse(getCameraViewMatrix());
  splitTransform(viewInv, Rstart, Tstart);
  flightInitialViewR = glm::dualquat_cast(Rstart);
  flightInitialViewT = Tstart;
  flightInitialFov = fov;

  // Final parameters
  glm::mat3x4 Rend;
  glm::vec3 Tend;
  glm::mat4 Tinv = glm::inverse(T);
  splitTransform(Tinv, Rend, Tend);
  flightTargetViewR = glm::dualquat_cast(Rend);
  flightTargetViewT = Tend;
  flightTargetFov = targetFov;

  midflight = true;
}

void immediatelyEndFlight() { midflight = false; }

void updateFlight() {

  // NOTE: we interpolate the _inverse_ view matrix (then invert back), because it looks better
  // when far from the origin

  if (midflight) {
    if (ImGui::GetTime() > flightEndTime) {
      // Flight is over, ensure we end exactly at target location
      midflight = false;
      viewMat = glm::inverse(buildTransform(glm::mat3x4_cast(flightTargetViewR), flightTargetViewT));
      fov = flightTargetFov;
    } else {
      // normalized time for spline on [0,1]
      float t = (ImGui::GetTime() - flightStartTime) / (flightEndTime - flightStartTime);

      float tSmooth = glm::smoothstep(0.f, 1.f, t);

      // linear spline
      glm::dualquat interpR = glm::lerp(flightInitialViewR, flightTargetViewR, tSmooth);
      glm::vec3 interpT = glm::mix(flightInitialViewT, flightTargetViewT, tSmooth);

      viewMat = glm::inverse(buildTransform(glm::mat3x4_cast(interpR), interpT));

      // linear spline
      fov = (1.0f - t) * flightInitialFov + t * flightTargetFov;
    }
    requestRedraw(); // flight is still happening, draw again next frame
  }
}

std::string getViewAsJson() {

  // Get the view matrix (note weird glm indexing, glm is [col][row])
  glm::mat4 viewMat = getCameraViewMatrix();
  std::array<float, 16> viewMatFlat;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      viewMatFlat[4 * i + j] = viewMat[j][i];
    }
  }

  // Build the json object
  json j = {
      {"fov", fov},
      {"viewMat", viewMatFlat},
      {"nearClip", nearClip},
      {"farClip", farClip},
      {"windowWidth", view::windowWidth},
      {"windowHeight", view::windowHeight},
      {"projectionMode", enum_to_string(view::projectionMode)},
      {"navigateStyle", enum_to_string(view::style)},
      {"upDir", enum_to_string(view::upDir)},
      {"frontDir", enum_to_string(view::frontDir)},
      {"viewRelativeMode", enum_to_string(view::viewRelativeMode)},
      {"viewCenter", {view::viewCenter.x, view::viewCenter.y, view::viewCenter.z}},
  };

  std::string outString = j.dump();
  return outString;
}
std::string getCameraJson() { return getViewAsJson(); }

void setViewFromJson(std::string jsonData, bool flyTo) {
  // Values will go here
  glm::mat4 newViewMat = viewMat;
  float newFov = fov;
  float newNearClipRatio = nearClip;
  float newFarClipRatio = farClip;

  int windowWidth = view::windowWidth;
  int windowHeight = view::windowHeight;

  try {

    // Deserialize
    json j;
    std::stringstream s(jsonData);
    s >> j;

    // Read out the data

    // Get the clip ratios, but only if present
    bool clipChanged = false;
    if (j.find("nearClip") != j.end()) {
      newNearClipRatio = j["nearClip"];
      clipChanged = true;
    }
    if (j.find("farClip") != j.end()) {
      newFarClipRatio = j["farClip"];
      clipChanged = true;
    }
    if (clipChanged) {
      setClipPlanes(newNearClipRatio, newFarClipRatio);
    }

    // Get the window sizes, if present
    bool windowSizeChanged = false;
    if (j.find("windowWidth") != j.end()) {
      windowWidth = j["windowWidth"];
      windowSizeChanged = true;
    }
    if (j.find("windowHeight") != j.end()) {
      windowHeight = j["windowHeight"];
      windowSizeChanged = true;
    }
    if (windowSizeChanged) {
      view::setWindowSize(windowWidth, windowHeight);
    }

    if (j.find("projectionMode") != j.end()) {
      std::string projectionModeStr = j["projectionMode"];
      ProjectionMode newProjectionMode;
      try_enum_from_string(projectionModeStr, newProjectionMode); // fail silently if unrecognized
      setProjectionMode(newProjectionMode);
    }

    if (j.find("navigateStyle") != j.end()) {
      std::string navigateStyleStr = j["navigateStyle"];
      NavigateStyle newStyle;
      if (try_enum_from_string(navigateStyleStr, newStyle)) {
        setNavigateStyle(newStyle, flyTo);
      }
    }

    if (j.find("upDir") != j.end()) {
      std::string upDirStr = j["upDir"];
      UpDir newUpDir;
      if (try_enum_from_string(upDirStr, newUpDir)) {
        updateViewAndChangeUpDir(newUpDir, flyTo);
      }
    }

    if (j.find("frontDir") != j.end()) {
      std::string frontDirStr = j["frontDir"];
      FrontDir newFrontDir;
      if (try_enum_from_string(frontDirStr, newFrontDir)) {
        updateViewAndChangeFrontDir(newFrontDir, flyTo);
      }
    }

    if (j.find("viewRelativeMode") != j.end()) {
      std::string viewRelativeModeStr = j["viewRelativeMode"];
      ViewRelativeMode newViewRelativeMode;
      if (try_enum_from_string(viewRelativeModeStr, newViewRelativeMode)) {
        setViewRelativeMode(newViewRelativeMode);
      }
    }

    if (j.find("viewCenter") != j.end()) {
      auto centerData = j["viewCenter"];
      if (centerData.size() == 3) {
        glm::vec3 newCenter;
        newCenter.x = centerData[0];
        newCenter.y = centerData[1];
        newCenter.z = centerData[2];
        setViewCenterRaw(newCenter);
      }
    }

    // NOTE: it's important that we do this after the mode/dir settings, as this
    // lets us do our flight
    bool viewChanged = false;
    if (j.find("fov") == j.end()) {
      newFov = j["fov"];
      viewChanged = true;
    }
    if (j.find("viewMat") != j.end()) {
      auto matData = j["viewMat"];
      if (matData.size() != 16) return; // check size
      auto it = matData.begin();
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
          newViewMat[j][i] = *it;
          it++;
        }
      }
      viewChanged = true;
    }
    if (viewChanged) {
      if (flyTo) {
        startFlightTo(newViewMat, fov);
      } else {
        setCameraViewMatrix(newViewMat);
        fov = newFov;
        requestRedraw();
      }
    }

  } catch (...) {
    // If anything goes wrong parsing, just give up
    return;
  }
}

void setCameraFromJson(std::string jsonData, bool flyTo) { setViewFromJson(jsonData, flyTo); }

void buildViewGui() {

  ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
  if (openSlicePlaneMenu) {
    // need to recursively open this tree node to respect slice plane menu open flag
    ImGui::SetNextItemOpen(true);
  }
  if (ImGui::TreeNode("View")) {

    // == Camera style

    std::string viewStyleName = enum_to_string(view::style);

    ImGui::PushItemWidth(120 * options::uiScale);
    std::array<NavigateStyle, 5> styles{NavigateStyle::Turntable, NavigateStyle::FirstPerson, NavigateStyle::Free,
                                        NavigateStyle::Planar, NavigateStyle::None};
    if (ImGui::BeginCombo("##View Style", viewStyleName.c_str())) {

      for (NavigateStyle s : styles) {
        if (ImGui::Selectable(enum_to_string(s).c_str(), view::style == s)) {
          setNavigateStyle(s, true);
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }
    ImGui::SameLine();

    ImGui::Text("Camera Style");

    { // == Up direction
      ImGui::PushItemWidth(120 * options::uiScale);
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
      ImGui::PushItemWidth(120 * options::uiScale);
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

    // Move speed
    float moveScaleF = view::moveScale;
    ImGui::SliderFloat(" Move Speed", &moveScaleF, 0.0, 2.0, "%.5f",
                       ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
    view::moveScale = moveScaleF;

    // Relative movement
    int viewRelativeModeInt = static_cast<int>(viewRelativeMode);
    if (ImGui::RadioButton("center relative##relMode", &viewRelativeModeInt,
                           static_cast<int>(ViewRelativeMode::CenterRelative))) {
      setViewRelativeMode(ViewRelativeMode::CenterRelative);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("length relative##relMode", &viewRelativeModeInt,
                           static_cast<int>(ViewRelativeMode::LengthRelative))) {
      setViewRelativeMode(ViewRelativeMode::LengthRelative);
    }

    // Show the center
    ImGui::Text("Center: <%.3g, %.3g, %.3g>", view::viewCenter.x, view::viewCenter.y, view::viewCenter.z);

    if (ImGui::TreeNode("Scene Extents")) {

      if (ImGui::Checkbox("Set automatically", &options::automaticallyComputeSceneExtents)) {
        updateStructureExtents();
      }

      ImGui::BeginDisabled(options::automaticallyComputeSceneExtents);


      static float lengthScaleUpper = -777;
      if (lengthScaleUpper == -777) lengthScaleUpper = 2. * state::lengthScale;
      if (ImGui::SliderFloat("Length Scale", &state::lengthScale, 0, lengthScaleUpper, "%.5f")) {
        requestRedraw();
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        // the upper bound for the slider is dynamically adjust to be a bit bigger than the lower bound, but only
        // does so on release of the widget (so it doesn't scaleo off to infinity), and only ever gets larger (so
        // you don't get stuck at 0)
        lengthScaleUpper = std::fmax(2. * state::lengthScale, lengthScaleUpper);
      }

      ImGui::TextUnformatted("Bounding Box:");
      ImGui::PushItemWidth(200 * options::uiScale);
      glm::vec3& bboxMin = std::get<0>(state::boundingBox);
      glm::vec3& bboxMax = std::get<1>(state::boundingBox);
      if (ImGui::InputFloat3("min", &bboxMin[0])) updateStructureExtents();
      if (ImGui::InputFloat3("max", &bboxMax[0])) updateStructureExtents();
      ImGui::PopItemWidth();

      ImGui::EndDisabled();


      ImGui::TreePop();
    }

    ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Camera Parameters")) {

      // Field of view
      float fovF = fov;
      if (ImGui::SliderFloat(" Field of View", &fovF, minFov, maxFov, "%.2f deg")) {
        fov = fovF;
        requestRedraw();
      };

      std::string projectionModeStr = enum_to_string(view::projectionMode);
      if (ImGui::BeginCombo("##ProjectionMode", projectionModeStr.c_str())) {
        if (ImGui::Selectable("Perspective", view::projectionMode == ProjectionMode::Perspective)) {
          setProjectionMode(ProjectionMode::Perspective);
          ImGui::SetItemDefaultFocus();
        }
        if (ImGui::Selectable("Orthographic", view::projectionMode == ProjectionMode::Orthographic)) {
          setProjectionMode(ProjectionMode::Orthographic);
          ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::SameLine();
      ImGui::Text("Projection");

      if (ImGui::TreeNode("Clip Planes")) {
        if (ImGui::SliderFloat("Near", &nearClip, 0., 10., "%.5f",
                               ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
          requestRedraw();
        }
        if (ImGui::SliderFloat("Far", &farClip, 1., 10000., "%.2f",
                               ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
          requestRedraw();
        }
        float absNearClip, absFarClip;
        std::tie(absNearClip, absFarClip) = computeClipPlanes();
        ImGui::TextUnformatted("Computed:");
        ImGui::Text("  near: %g", absNearClip);
        ImGui::Text("  far: %g", absFarClip);

        ImGui::TreePop();
      }

      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Window")) {

      {
        ImGui::TextUnformatted("Dim:");
        ImGui::SameLine();
        ImGui::PushItemWidth(50 * options::uiScale);
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

void setUpDir(UpDir newUpDir, bool animateFlight) { updateViewAndChangeUpDir(newUpDir, animateFlight); }

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

void setFrontDir(FrontDir newFrontDir, bool animateFlight) { updateViewAndChangeFrontDir(newFrontDir, animateFlight); }

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


void setNavigateStyle(NavigateStyle newStyle, bool animateFlight) {
  updateViewAndChangeNavigationStyle(newStyle, animateFlight);
}
NavigateStyle getNavigateStyle() { return style; }

void setWindowResizable(bool isResizable) {
  windowResizable = isResizable;
  if (isInitialized()) {
    return render::engine->setWindowResizable(isResizable);
  }
}

bool getWindowResizable() { return windowResizable; }


} // namespace view
} // namespace polyscope
