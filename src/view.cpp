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
int& activeViewportIndex = state::globalContext.activeViewportIndex;


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

std::tuple<int, int> screenCoordsToBufferIndsViewport(glm::vec2 screenCoords, int32_t viewportInd) {
  if (viewportInd < 0) viewportInd = getActiveViewportIndex();
  const ViewParameters& vp = getViewportParameters(viewportInd);
  glm::ivec2 globalInds = screenCoordsToBufferIndsVec(screenCoords);
  // globalInds.y is y-from-top (screenCoordsToBufferIndsVec just scales without flipping).
  // viewportPosY is y-from-bottom. Compute the viewport's top in y-from-top buffer coords first.
  int svTopFromTopBuf = bufferHeight - vp.viewportPosY - vp.viewportBufferHeight;
  int localX = std::max(std::min(globalInds.x - vp.viewportPosX, vp.viewportBufferWidth - 1), 0);
  int localY = std::max(std::min(globalInds.y - svTopFromTopBuf, vp.viewportBufferHeight - 1), 0);
  return std::tuple<int, int>(localX, localY);
}

glm::ivec2 screenCoordsToBufferIndsVecViewport(glm::vec2 screenCoords, int32_t viewportInd) {
  glm::ivec2 out;
  std::tie(out.x, out.y) = screenCoordsToBufferIndsViewport(screenCoords, viewportInd);
  return out;
}

glm::vec2 bufferIndsToScreenCoordsViewport(int xPos, int yPos, int32_t viewportInd) {
  if (viewportInd < 0) viewportInd = getActiveViewportIndex();
  const ViewParameters& vp = getViewportParameters(viewportInd);
  // xPos/yPos are local coords within the viewport (y-from-top).
  // viewportPosY is y-from-bottom. Compute the viewport's top in y-from-top buffer coords.
  int svTopFromTopBuf = bufferHeight - vp.viewportPosY - vp.viewportBufferHeight;
  return bufferIndsToScreenCoords(vp.viewportPosX + xPos, svTopFromTopBuf + yPos);
}

glm::vec2 bufferIndsToScreenCoordsViewport(glm::ivec2 bufferInds, int32_t viewportInd) {
  return bufferIndsToScreenCoordsViewport(bufferInds.x, bufferInds.y, viewportInd);
}

std::tuple<glm::vec2, int32_t> screenCoordsGlobalToViewport(glm::vec2 screenCoords) {
  int32_t iViewport = screenCoordsToViewportInd(screenCoords);
  glm::ivec2 localInds = screenCoordsToBufferIndsVecViewport(screenCoords, iViewport);
  glm::vec2 localCoords = bufferIndsToScreenCoords(localInds);
  return {localCoords, iViewport};
}

std::tuple<glm::ivec2, int32_t> bufferIndsGlobalToViewport(glm::ivec2 bufferInds) {
  int32_t iViewport = bufferIndsToViewportInd(bufferInds);
  glm::ivec2 localInds = screenCoordsToBufferIndsVecViewport(bufferIndsToScreenCoords(bufferInds), iViewport);
  return {localInds, iViewport};
}

glm::vec2 screenCoordsViewportToGlobal(glm::vec2 screenCoords, int32_t iViewport) {
  glm::ivec2 localInds = screenCoordsToBufferIndsVec(screenCoords);
  glm::vec2 globalScreenCoords = bufferIndsToScreenCoordsViewport(localInds.x, localInds.y, iViewport);
  return globalScreenCoords;
}

glm::ivec2 bufferIndsViewportToGlobal(glm::ivec2 bufferInds, int32_t iViewport) {
  if (iViewport < 0) iViewport = getActiveViewportIndex();
  const ViewParameters& vp = getViewportParameters(iViewport);
  int svTopFromTopBuf = bufferHeight - vp.viewportPosY - vp.viewportBufferHeight;
  return glm::ivec2(vp.viewportPosX + bufferInds.x, svTopFromTopBuf + bufferInds.y);
}

int32_t bufferIndsToViewportInd(glm::ivec2 bufferInds) {
  for (int32_t i = getNumViewports() - 1; i >= 0; i--) {
    // go back-to-front, so we report the topmost matching viewport
    const ViewParameters& vp = getViewportParameters(i);
    if (bufferInds.x >= vp.viewportPosX && bufferInds.x < vp.viewportPosX + vp.viewportBufferWidth &&
        bufferInds.y >= vp.viewportPosY && bufferInds.y < vp.viewportPosY + vp.viewportBufferHeight) {
      return i;
    }
  }

  return 0; // returning 0 is the least evil option if we miss somehow due to cracks or invalid input
}

int32_t screenCoordsToViewportInd(glm::vec2 screenCoords) {
  // screenCoords.y is from the top (ImGui etc convention); viewportPosY is from the bottom (OpenGL convention)
  glm::vec2 flipped{screenCoords.x, windowHeight - screenCoords.y};
  return bufferIndsToViewportInd(screenCoordsToBufferIndsVec(flipped));
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

  immediatelyEndFlight();
  propagateViewUpdatesToViewports();
  requestRedraw();
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

  immediatelyEndFlight();
  propagateViewUpdatesToViewports();
  requestRedraw();
}

void processClipPlaneShift(float amount) {
  if (amount == 0.0) return;
  // Adjust the near clipping plane
  nearClip += .03 * amount * nearClip;
  propagateViewUpdatesToViewports();
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
  propagateViewUpdatesToViewports();
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
    propagateViewUpdatesToViewports();
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

  propagateViewUpdatesToViewports();
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

  propagateViewUpdatesToViewports();
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

  propagateViewUpdatesToViewports();
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

  propagateViewUpdatesToViewports();
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
    propagateViewUpdatesToViewports();
    startFlightTo(targetView, fov);
  } else {
    viewMat = targetView;
    projectCenterToBeValidForView();
    propagateViewUpdatesToViewports();
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
  propagateViewUpdatesToViewports();
  requestRedraw();
}

CameraParameters getCameraParametersForCurrentView() {
  ensureViewValid();

  float aspectRatio = getAspectRatioWidthOverHeight();
  return CameraParameters(CameraIntrinsics::fromFoVDegVerticalAndAspect(fov, aspectRatio),
                          CameraExtrinsics::fromMatrix(viewMat));
}

void setCameraViewMatrix(glm::mat4 mat) {
  viewMat = mat;
  projectCenterToBeValidForView();
  propagateViewUpdatesToViewports();
  requestRedraw();
}

glm::mat4 getCameraViewMatrix() { return viewMat; }

void setVerticalFieldOfViewDegrees(float newVal) {
  view::fov = newVal;
  propagateViewUpdatesToViewports();
  requestRedraw();
}

ProjectionMode getProjectionMode() { return projectionMode; }

void setProjectionMode(ProjectionMode newMode) {
  projectionMode = newMode;
  propagateViewUpdatesToViewports();
  requestRedraw();
}



ViewRelativeMode getViewRelativeMode() { return viewRelativeMode; }
void setViewRelativeMode(ViewRelativeMode newMode) {
  viewRelativeMode = newMode;
  propagateViewUpdatesToViewports();
  requestRedraw();
}
void setClipPlanes(float newNearClip, float newFarClip) {
  nearClip = newNearClip;
  farClip = newFarClip;
  propagateViewUpdatesToViewports();
  requestRedraw();
}
std::tuple<float, float> getClipPlanes() { return std::tuple<float, float>(nearClip, farClip); }

float getVerticalFieldOfViewDegrees() { return view::fov; }

float getAspectRatioWidthOverHeight() { return (float)getActiveViewportBufferWidth() / getActiveViewportBufferHeight(); }

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
  float aspectRatio = getAspectRatioWidthOverHeight();
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

// Build a perspective matrix for a specific viewport (may differ from the active viewport).
// Reads projectionMode and fov directly from the viewport parameters.
static glm::mat4 getCameraPerspectiveMatrixForViewport(int32_t viewportInd) {
  float absNearClip, absFarClip;
  if (overrideClipPlanes) {
    absNearClip = overrideNearClipRelative * state::lengthScale;
    absFarClip = overrideFarClipRelative * state::lengthScale;
  } else {
    std::tie(absNearClip, absFarClip) = computeClipPlanes();
  }

  const ViewParameters& vp = state::globalContext.viewportParameters[viewportInd];
  float fovRad = glm::radians(vp.fov);
  float aspectRatio = (float)vp.viewportBufferWidth / vp.viewportBufferHeight;
  switch (vp.projectionMode) {
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

glm::vec3 screenCoordsToWorldRay(glm::vec2 screenCoords, int32_t viewportInd) {
  if (viewportInd < 0) viewportInd = screenCoordsToViewportInd(screenCoords);
  const ViewParameters& vp = getViewportParameters(viewportInd);

  glm::mat4 viewMat = getCameraViewMatrix();
  glm::mat4 proj = getCameraPerspectiveMatrixForViewport(viewportInd);
  glm::vec4 viewport = {0., 0., vp.viewportWindowWidth, vp.viewportWindowHeight};

  // Convert global screen coords (y-from-top) to viewport-local.
  // viewportRelativePosY is y-from-bottom, so the top of the viewport in y-from-top is:
  // (1 - posY - height) * windowHeight
  float localX = screenCoords.x - windowWidth * vp.viewportRelativePosX;
  float svTopFromTop = (1.f - vp.viewportRelativePosY - vp.viewportRelativeHeight) * windowHeight;
  float localY = screenCoords.y - svTopFromTop;

  glm::vec3 screenPos3{localX, vp.viewportWindowHeight - localY, 0.};
  glm::vec3 worldPos = glm::unProject(screenPos3, viewMat, proj, viewport);
  glm::vec3 worldRayDir = glm::normalize(glm::vec3(worldPos) - getCameraWorldPosition());

  return worldRayDir;
}

glm::vec3 bufferIndsToWorldRay(glm::ivec2 bufferInds, int32_t viewportInd) {
  return bufferCoordsToWorldRay(glm::vec2(bufferInds), viewportInd);
}

glm::vec3 bufferCoordsToWorldRay(glm::vec2 bufferCoords, int32_t viewportInd) {
  if (viewportInd < 0) viewportInd = getActiveViewportIndex();
  const ViewParameters& vp = getViewportParameters(viewportInd);

  glm::mat4 view = getCameraViewMatrix();
  glm::mat4 proj = getCameraPerspectiveMatrixForViewport(viewportInd);
  glm::vec4 viewport = {0., 0., vp.viewportBufferWidth, vp.viewportBufferHeight};

  glm::vec3 screenPos3{bufferCoords.x, vp.viewportBufferHeight - bufferCoords.y, 0.};
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

glm::vec3 screenCoordsAndDepthToWorldPosition(glm::vec2 screenCoords, float clipDepth, int32_t viewportInd) {

  if (clipDepth == 1.) {
    // if we didn't hit anything in the depth buffer, just return infinity
    float inf = std::numeric_limits<float>::infinity();
    return glm::vec3{inf, inf, inf};
  }

  if (viewportInd < 0) viewportInd = screenCoordsToViewportInd(screenCoords);
  const ViewParameters& vp = getViewportParameters(viewportInd);

  glm::mat4 view = getCameraViewMatrix();
  glm::mat4 viewInv = glm::inverse(view);
  glm::mat4 proj = getCameraPerspectiveMatrixForViewport(viewportInd);
  glm::mat4 projInv = glm::inverse(proj);
  // glm::vec2 depthRange = {0., 1.}; // no support for nonstandard depth range, currently

  // Convert global screen coords (y-from-top) to viewport-local, then normalize to [0,1].
  // viewportRelativePosY is y-from-bottom, so the top of the viewport in y-from-top is:
  // (1 - posY - height) * windowHeight
  float localX = screenCoords.x - windowWidth * vp.viewportRelativePosX;
  float svTopFromTop = (1.f - vp.viewportRelativePosY - vp.viewportRelativeHeight) * windowHeight;
  float localY = screenCoords.y - svTopFromTop;
  glm::vec2 screenPos{localX / static_cast<float>(vp.viewportWindowWidth),
                      1.f - localY / static_cast<float>(vp.viewportWindowHeight)};
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

  propagateViewUpdatesToViewports();
  requestRedraw();
}

void immediatelyEndFlight() {
  midflight = false;
  propagateViewUpdatesToViewports();
  requestRedraw();
}

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
    if (j.find("fov") != j.end()) {
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
        startFlightTo(newViewMat, newFov);
      } else {
        setCameraViewMatrix(newViewMat);
        fov = newFov;
        requestRedraw();
      }
    }

    propagateViewUpdatesToViewports();

  } catch (...) {
    // If anything goes wrong parsing, just give up
    propagateViewUpdatesToViewports();
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
        propagateViewUpdatesToViewports();
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
          propagateViewUpdatesToViewports();
          requestRedraw();
        }
        if (ImGui::SliderFloat("Far", &farClip, 1., 10000., "%.2f",
                               ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
          propagateViewUpdatesToViewports();
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


// == Viewport helpers

namespace {
void packCurrentViewportParametersToIndex(int idx) {
  if (idx < 0) return;

  ViewParameters& vp = state::globalContext.viewportParameters[idx];
  vp.style = style;
  vp.upDir = upDir;
  vp.frontDir = frontDir;
  vp.moveScale = moveScale;
  vp.viewRelativeMode = viewRelativeMode;
  vp.nearClip = nearClip;
  vp.farClip = farClip;
  vp.bgColor = bgColor;
  vp.viewMat = viewMat;
  vp.fov = fov;
  vp.viewCenter = viewCenter;
  vp.projectionMode = projectionMode;
  vp.midflight = midflight;
  vp.flightStartTime = flightStartTime;
  vp.flightEndTime = flightEndTime;
  vp.flightTargetViewR = flightTargetViewR;
  vp.flightInitialViewR = flightInitialViewR;
  vp.flightTargetViewT = flightTargetViewT;
  vp.flightInitialViewT = flightInitialViewT;
  vp.flightTargetFov = flightTargetFov;
  vp.flightInitialFov = flightInitialFov;
}

void unpackCurrentViewportParametersFromActive() {
  if (activeViewportIndex < 0) return;

  const ViewParameters& vp = state::globalContext.viewportParameters[activeViewportIndex];
  style = vp.style;
  upDir = vp.upDir;
  frontDir = vp.frontDir;
  moveScale = vp.moveScale;
  viewRelativeMode = vp.viewRelativeMode;
  nearClip = vp.nearClip;
  farClip = vp.farClip;
  bgColor = vp.bgColor;
  viewMat = vp.viewMat;
  fov = vp.fov;
  viewCenter = vp.viewCenter;
  projectionMode = vp.projectionMode;
  midflight = vp.midflight;
  flightStartTime = vp.flightStartTime;
  flightEndTime = vp.flightEndTime;
  flightTargetViewR = vp.flightTargetViewR;
  flightInitialViewR = vp.flightInitialViewR;
  flightTargetViewT = vp.flightTargetViewT;
  flightInitialViewT = vp.flightInitialViewT;
  flightTargetFov = vp.flightTargetFov;
  flightInitialFov = vp.flightInitialFov;
}

} // namespace


int getActiveViewportIndex() { return activeViewportIndex; }

int getActiveViewportBufferWidth() {
  return state::globalContext.viewportParameters[activeViewportIndex].viewportBufferWidth;
}

int getActiveViewportBufferHeight() {
  return state::globalContext.viewportParameters[activeViewportIndex].viewportBufferHeight;
}

glm::vec4 getViewportImGuiRect(int32_t viewportInd) {
  if (viewportInd < 0) viewportInd = activeViewportIndex;
  const ViewParameters& vp = getViewportParameters(viewportInd);
  float x = vp.viewportRelativePosX * windowWidth;
  float y = (1.f - vp.viewportRelativePosY - vp.viewportRelativeHeight) * windowHeight;
  return glm::vec4(x, y, vp.viewportWindowWidth, vp.viewportWindowHeight);
}

void activateViewport(int viewportInd) {
  if (viewportInd < 0 || viewportInd >= getNumViewports()) {
    error("Invalid viewport index " + std::to_string(viewportInd));
  }
  packCurrentViewportParametersToIndex(activeViewportIndex);
  activeViewportIndex = viewportInd;
  unpackCurrentViewportParametersFromActive();
}

int getNumViewports() { return state::globalContext.viewportParameters.size(); }

void setNumViewports(int count) {
  checkInitialized();
  if (count <= 0) error("Invalid viewport count " + std::to_string(count));
  if (count > 64) error("At most 64 viewports are supported.");


  // save current parameters
  if (getNumViewports() > 0) {
    packCurrentViewportParametersToIndex(activeViewportIndex);
  }

  int oldCount = getNumViewports();
  state::globalContext.viewportParameters.resize(count);

  // Clamp active index to valid range after shrinking
  if (activeViewportIndex >= count) activeViewportIndex = count - 1;

  // Copy the current active view parameters into all new views to initialize them
  for (int i = oldCount; i < count; i++) packCurrentViewportParametersToIndex(i);

  render::engine->ensureSceneTargetsAllocated(count);

  requestRedraw();
}

void setActiveViewportRelativeLocation(float posX, float posY) {
  checkInitialized();
  if (posX < 0 || posX > 1 || posY < 0 || posY > 1) {
    error("Invalid viewport location " + std::to_string(posX) + " x " + std::to_string(posY));
  }
  state::globalContext.viewportParameters[activeViewportIndex].viewportRelativePosX = posX;
  state::globalContext.viewportParameters[activeViewportIndex].viewportRelativePosY = posY;
  render::engine->resizeScreenBuffersAndSetViewports();
  requestRedraw();
}

void setActiveViewportRelativeSize(float width, float height) {
  checkInitialized();
  if (width < 0 || width > 1 || height < 0 || height > 1) {
    error("Invalid viewport size " + std::to_string(width) + " x " + std::to_string(height));
  }
  state::globalContext.viewportParameters[activeViewportIndex].viewportRelativeWidth = width;
  state::globalContext.viewportParameters[activeViewportIndex].viewportRelativeHeight = height;
  render::engine->resizeScreenBuffersAndSetViewports();
  requestRedraw();
}

bool getViewportsShareNavigation() { return state::globalContext.viewportsShareNavigation; }

void setViewportsShareNavigation(bool shareNavigation) {
  state::globalContext.viewportsShareNavigation = shareNavigation;
  propagateViewUpdatesToViewports();
  requestRedraw();
}

void createViewportGrid(int gridWidth, int gridHeight) {
  // make sure the dimensions are reasonable
  if (gridWidth <= 0 || gridHeight <= 0) {
    error("Invalid viewport grid dimensions " + std::to_string(gridWidth) + " x " + std::to_string(gridHeight));
  }

  setNumViewports(gridWidth * gridHeight);

  // initialize all viewports to the same parameters as the current active view, so they start in a known state
  for (int i = 0; i < getNumViewports(); i++) {
    packCurrentViewportParametersToIndex(i);
  }

  float cellW = 1.0f / gridWidth;
  float cellH = 1.0f / gridHeight;
  for (int i = 0; i < gridHeight; i++) {
    for (int j = 0; j < gridWidth; j++) {
      int ind = i * gridWidth + j;
      activateViewport(ind);
      // note: this intentionally puts the first grid subiew in the upper-left, matching convention from other software
      setActiveViewportRelativeLocation(static_cast<float>(j) / gridWidth,
                                       static_cast<float>(gridHeight - i - 1) / gridHeight);
      setActiveViewportRelativeSize(cellW, cellH);
    }
  }

  activateViewport(0);
  render::engine->resizeScreenBuffersAndSetViewports();
  requestRedraw();
}

void createInitialViewport() {
  state::globalContext.viewportParameters.resize(1);
  packCurrentViewportParametersToIndex(0);
  activeViewportIndex = 0;
  state::globalContext.viewportParameters[0].viewportRelativePosX = 0.0;
  state::globalContext.viewportParameters[0].viewportRelativePosY = 0.0;
  state::globalContext.viewportParameters[0].viewportRelativeWidth = 1.0;
  state::globalContext.viewportParameters[0].viewportRelativeHeight = 1.0;
  if (!render::engine) error("Render engine not initialized. You called an internal function at the wrong time.");
  render::engine->ensureSceneTargetsAllocated(1);
  render::engine->setActiveSceneTarget(0);
}

void clearViewports() {
  state::globalContext.viewportParameters.clear();
  activeViewportIndex = -1;
}

void validateViewports() {
  if (getNumViewports() == 0)
    error("No viewports of the scene exist. Either viewports were configured wrong, program flow has been called in the "
          "wrong order.");
}

void updateViewportDimensions() {
  packCurrentViewportParametersToIndex(activeViewportIndex); // active values are duplicated
  for (int32_t iV = 0; iV < getNumViewports(); iV++) {
    ViewParameters& vp = state::globalContext.viewportParameters[iV];

    // Compute pos and size by rounding both edges independently, so that adjacent viewports sharing
    // a boundary always snap to the same integer pixel (no cracks or overlaps).
    vp.viewportPosX = (int)std::round(view::bufferWidth * vp.viewportRelativePosX);
    vp.viewportPosY = (int)std::round(view::bufferHeight * vp.viewportRelativePosY);
    vp.viewportBufferWidth =
        (int)std::round(view::bufferWidth * (vp.viewportRelativePosX + vp.viewportRelativeWidth)) - vp.viewportPosX;
    vp.viewportBufferHeight =
        (int)std::round(view::bufferHeight * (vp.viewportRelativePosY + vp.viewportRelativeHeight)) - vp.viewportPosY;
    vp.viewportWindowWidth = (int)std::round(view::windowWidth * (vp.viewportRelativePosX + vp.viewportRelativeWidth)) -
                            (int)std::round(view::windowWidth * vp.viewportRelativePosX);
    vp.viewportWindowHeight = (int)std::round(view::windowHeight * (vp.viewportRelativePosY + vp.viewportRelativeHeight)) -
                             (int)std::round(view::windowHeight * vp.viewportRelativePosY);
  }
  unpackCurrentViewportParametersFromActive();
}

void propagateViewUpdatesToViewports() {
  if (!getViewportsShareNavigation()) return;

  for (int32_t iV = 0; iV < getNumViewports(); iV++) {
    ViewParameters& vp = state::globalContext.viewportParameters[iV];
    vp.style = style;
    vp.upDir = upDir;
    vp.frontDir = frontDir;
    vp.moveScale = moveScale;
    vp.viewRelativeMode = viewRelativeMode;
    vp.nearClip = nearClip;
    vp.farClip = farClip;
    vp.bgColor = bgColor;
    vp.viewMat = viewMat;
    vp.fov = fov;
    vp.viewCenter = viewCenter;
    vp.projectionMode = projectionMode;
    vp.midflight = midflight;
    vp.flightStartTime = flightStartTime;
    vp.flightEndTime = flightEndTime;
    vp.flightTargetViewR = flightTargetViewR;
    vp.flightInitialViewR = flightInitialViewR;
    vp.flightTargetViewT = flightTargetViewT;
    vp.flightInitialViewT = flightInitialViewT;
    vp.flightTargetFov = flightTargetFov;
    vp.flightInitialFov = flightInitialFov;
  }
}

const ViewParameters& getViewportParameters(int viewportInd) {
  if (viewportInd < 0) viewportInd = activeViewportIndex;
  if (viewportInd >= getNumViewports()) {
    error("Invalid viewport index " + std::to_string(viewportInd));
  }
  if (viewportInd == activeViewportIndex) {
    // write them to the structure in case they have been changed since it was activated
    packCurrentViewportParametersToIndex(activeViewportIndex);
  }
  return state::globalContext.viewportParameters[viewportInd];
}

} // namespace view
} // namespace polyscope
