// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <array>
#include <string>

#include "polyscope/camera_parameters.h"
#include "polyscope/types.h"
// #include "polyscope/gl/gl_utils.h"

// GLM for view matrices
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/dual_quaternion.hpp"
#include "glm/gtx/norm.hpp" // necessary for dual_quaternion below
#undef GLM_ENABLE_EXPERIMENTAL

namespace polyscope {
namespace view {

// Previously, these enums were defined here in the `view` namespace, but now for consistency we define all enums in the
// top-level `polyscope` namespace. For backwards compatability, we import the enums so existing code like
// polyscope::view::NavigateStyle::Planar still works.
using polyscope::NavigateStyle;
using polyscope::UpDir;

// === View state
extern int bufferWidth;
extern int bufferHeight;
extern int windowWidth;
extern int windowHeight;
extern int initWindowPosX;
extern int initWindowPosY;
extern NavigateStyle style;
extern UpDir upDir;
extern FrontDir frontDir;
extern double moveScale;
extern double nearClipRatio;
extern double farClipRatio;
extern std::array<float, 4> bgColor;

// Current view camera parameters
// TODO deprecate these one day, and just use a CameraParameters member instead. But this would break existing code, so
// for now we leave these as-is and wrap inputs/outputs to a CameraParameters
extern glm::mat4x4 viewMat;
extern double fov; // in the y direction
extern ProjectionMode projectionMode;

// "Flying" view
extern bool midflight;
extern float flightStartTime;
extern float flightEndTime;
extern glm::dualquat flightTargetViewR, flightInitialViewR;
extern glm::vec3 flightTargetViewT, flightInitialViewT;
extern float flightTargetFov, flightInitialFov;

// Default values
extern const double defaultNearClipRatio;
extern const double defaultFarClipRatio;
extern const double defaultFov;

// === View methods

void processTranslate(glm::vec2 delta);
void processRotate(glm::vec2 startP, glm::vec2 endP);

void processClipPlaneShift(double amount);
void processZoom(double amount);

void setWindowSize(int width, int height);
std::tuple<int, int> getWindowSize();
std::tuple<int, int> getBufferSize();
void setViewToCamera(const CameraParameters& p);
CameraParameters getCameraParametersForCurrentView();

// Invalidating the view:
// The view is invalid if the viewMat has NaN entries.
// It is set to invalid initially, but we call ensureViewValid() before any renders.
// This ensures we never try to render with an invalid view, but also allows the user to
// set custom views if they wish, without them getting overwritten.
void invalidateView();
void ensureViewValid();

// The "home" view looks at the center of the scene's bounding box.
glm::mat4 computeHomeView();
void resetCameraToHomeView();
void flyToHomeView();

// Set the camera extrinsics to look at a particular location
void lookAt(glm::vec3 cameraLocation, glm::vec3 target, bool flyTo = false);
void lookAt(glm::vec3 cameraLocation, glm::vec3 target, glm::vec3 upDir, bool flyTo = false);

// Get various camera matrices and data for the current view
glm::mat4 getCameraViewMatrix();
void setCameraViewMatrix(glm::mat4 newMat);
glm::mat4 getCameraPerspectiveMatrix();
glm::vec3 getCameraWorldPosition();
void getCameraFrame(glm::vec3& lookDir, glm::vec3& upDir, glm::vec3& rightDir);

// Get world geometry corresponding to a screen pixel (e.g. from a mouse click)
glm::vec3 screenCoordsToWorldRay(glm::vec2 screenCoords);
glm::vec3 bufferCoordsToWorldRay(glm::vec2 screenCoords);
glm::vec3 screenCoordsToWorldPosition(glm::vec2 screenCoords); // queries the depth buffer to get full position

// Flight-related
void startFlightTo(const CameraParameters& p, float flightLengthInSeconds = .4);
void startFlightTo(const glm::mat4& T, float targetFov, float flightLengthInSeconds = .4);
void immediatelyEndFlight();

// Get and set camera from json string
std::string getViewAsJson();
void setViewFromJson(std::string jsonData, bool flyTo);
// DEPRACTED: old names for avove
std::string getCameraJson();
void setCameraFromJson(std::string jsonData, bool flyTo);

// Internal helpers. Should probably not be called in user code.
void buildViewGui();
void updateFlight(); // Note: uses wall-clock time, so should generally be called exactly once at the beginning of each
                     // iteration


// == Setters, getters, etc

void setUpDir(UpDir newUpDir, bool animateFlight = false);
UpDir getUpDir();
glm::vec3 getUpVec();

void setFrontDir(FrontDir newFrontDir, bool animateFlight = false);
FrontDir getFrontDir();
glm::vec3 getFrontVec();

void setNavigateStyle(NavigateStyle newNavigateStyle, bool animateFlight = false);
NavigateStyle getNavigateStyle();

void setWindowResizable(bool isResizable);
bool getWindowResizable();

} // namespace view
} // namespace polyscope
