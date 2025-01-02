// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <array>
#include <string>
#include <tuple>

#include "polyscope/camera_parameters.h"
#include "polyscope/types.h"
// #include "polyscope/gl/gl_utils.h"

#include "imgui.h"

// GLM for view matrices
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/dual_quaternion.hpp"
#include "glm/gtx/norm.hpp" // necessary for dual_quaternion below
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace polyscope {
namespace view {

// Previously, these enums were defined here in the `view` namespace, but now for consistency we define all enums in the
// top-level `polyscope` namespace. For backwards compatability, we import the enums so existing code like
// polyscope::view::NavigateStyle::Planar still works.
using polyscope::NavigateStyle;
using polyscope::UpDir;

// === View state

// NOTE: users should use setters to set these if they exist, otherwise updates
// may not be applied immediately.
extern int& bufferWidth;
extern int& bufferHeight;
extern int& windowWidth;
extern int& windowHeight;
extern int& initWindowPosX;
extern int& initWindowPosY;
extern bool& windowResizable;
extern NavigateStyle& style;
extern UpDir& upDir;
extern FrontDir& frontDir;
extern double& moveScale;
extern double& nearClipRatio;
extern double& farClipRatio;
extern std::array<float, 4>& bgColor;

// Current view camera parameters
// TODO deprecate these one day, and just use a CameraParameters member instead. But this would break existing code, so
// for now we leave these as-is and wrap inputs/outputs to a CameraParameters
extern glm::mat4x4& viewMat;
extern double& fov; // in the y direction
extern ProjectionMode& projectionMode;

// "Flying" view members
extern bool& midflight;
extern float& flightStartTime;
extern float& flightEndTime;
extern glm::dualquat& flightTargetViewR;
extern glm::dualquat& flightInitialViewR;
extern glm::vec3& flightTargetViewT;
extern glm::vec3& flightInitialViewT;
extern float& flightTargetFov;
extern float& flightInitialFov;

// Default values
extern const int defaultWindowWidth;
extern const int defaultWindowHeight;
extern const double defaultNearClipRatio;
extern const double defaultFarClipRatio;
extern const double defaultFov;

// === View methods

// == Get/Set the current camera view in the user's window

// Get various camera matrices and data for the current view
CameraParameters getCameraParametersForCurrentView(); // contains all of this info
// (these friendly helpers to get the same info as ^^^)
glm::mat4 getCameraViewMatrix();
void setCameraViewMatrix(glm::mat4 newMat);
glm::mat4 getCameraPerspectiveMatrix();
glm::vec3 getCameraWorldPosition();
void getCameraFrame(glm::vec3& lookDir, glm::vec3& upDir, glm::vec3& rightDir);
glm::vec3 getUpVec();
glm::vec3 getFrontVec();

// Set the camera extrinsics to look at a particular location
void setViewToCamera(const CameraParameters& p);
void lookAt(glm::vec3 cameraLocation, glm::vec3 target, bool flyTo = false);
void lookAt(glm::vec3 cameraLocation, glm::vec3 target, glm::vec3 upDir, bool flyTo = false);

// The "home" view looks at the center of the scene's bounding box.
glm::mat4 computeHomeView();
void resetCameraToHomeView();
void flyToHomeView();

// Move the camera with a 'flight' where the camera's position is briefly animated
void startFlightTo(const CameraParameters& p, float flightLengthInSeconds = .4);
void startFlightTo(const glm::mat4& T, float targetFov, float flightLengthInSeconds = .4);
void immediatelyEndFlight();


// == Properties of the view/window

// Set the size of the OS window
// Set in logical pixels, which might be different from actual buffer pixels on
// high-DPI screens
void setWindowSize(int width, int height);
std::tuple<int, int> getWindowSize();
std::tuple<int, int> getBufferSize();

// UpDir is the canonical up-axis for the scene, effects how the home view is oriented,
// and the axis about which navigations like the default turntable rotates.
void setUpDir(UpDir newUpDir, bool animateFlight = false);
UpDir getUpDir();

// FrontDir is the canonical forward-axis for the scene, effects how the home view is oriented
void setFrontDir(FrontDir newFrontDir, bool animateFlight = false);
FrontDir getFrontDir();

// What kind of navigation is used, such as Turntable, Free, etc.
void setNavigateStyle(NavigateStyle newNavigateStyle, bool animateFlight = false);
NavigateStyle getNavigateStyle();

// Can the OS window be resized by the user?
void setWindowResizable(bool isResizable);
bool getWindowResizable();


// == Utility functions related to the view

// Get world geometry corresponding to a screen pixel (e.g. from a mouse click)
glm::vec3 screenCoordsToWorldRay(glm::vec2 screenCoords);
glm::vec3 bufferCoordsToWorldRay(int xPos, int yPos);
glm::vec3 screenCoordsToWorldPosition(glm::vec2 screenCoords); // queries the depth buffer to get full position

// Get and set camera from json string
std::string getViewAsJson();
void setViewFromJson(std::string jsonData, bool flyTo);
std::string getCameraJson(); // DEPRACTED: old names for avove
void setCameraFromJson(std::string jsonData, bool flyTo);

// Misc helpers
std::string to_string(ProjectionMode mode);
std::string to_string(NavigateStyle style);
std::tuple<int, int> screenCoordsToBufferInds(glm::vec2 screenCoords);

// == Internal helpers. Should probably not be called in user code.

// Build view-related ImGUI UI
void buildViewGui();

// Update the current flight animation, if there is one
// Note: uses wall-clock time, should be called exactly once at the beginning of each iteration
void updateFlight();

// Invalidating the view:
// The view is invalid if the viewMat has NaN entries.
// It is set to invalid initially, but we call ensureViewValid() before any renders.
// This ensures we never try to render with an invalid view, but also allows the user to
// set custom views if they wish, without them getting overwritten.
void invalidateView();
void ensureViewValid();

// Process user inputs which affect the view
void processTranslate(glm::vec2 delta);
void processRotate(glm::vec2 startP, glm::vec2 endP);
void processClipPlaneShift(double amount);
void processZoom(double amount);
void processKeyboardNavigation(ImGuiIO& io);


} // namespace view
} // namespace polyscope
