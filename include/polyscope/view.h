// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <array>
#include <string>
#include <tuple>

#include "polyscope/camera_parameters.h"
#include "polyscope/types.h"
#include "polyscope/view_parameters.h"
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
extern float& moveScale;
extern ViewRelativeMode& viewRelativeMode;
extern float& nearClip;
extern float& farClip;
extern std::array<float, 4>& bgColor;

// Current view camera parameters
// TODO deprecate these one day, and just use a CameraParameters member instead. But this would break existing code, so
// for now we leave these as-is and wrap inputs/outputs to a CameraParameters
extern glm::mat4x4& viewMat;
extern float& fov; // in the y direction
extern ProjectionMode& projectionMode;
extern glm::vec3& viewCenter; // center about which view transformations are performed

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
extern const float defaultNearClipRatio;
extern const float defaultFarClipRatio;
extern const float defaultFov;

// Internal details
extern int& activeViewportIndex;
extern bool overrideClipPlanes; // used for temporary state changes in render passes, internal use only!
extern float overrideNearClipRelative;
extern float overrideFarClipRelative;

// === View methods

// == Get/Set the current camera view in the user's window

// If using viewports, whenever not specified, all functions apply to the current
// active viewport.

// Get various camera matrices and data for the current view
CameraParameters getCameraParametersForCurrentView(); // contains all of this info
// (these friendly helpers to get the same info as ^^^)
glm::mat4 getCameraViewMatrix();
void setCameraViewMatrix(glm::mat4 newMat);
ProjectionMode getProjectionMode();
void setProjectionMode(ProjectionMode newMode);
glm::mat4 getCameraPerspectiveMatrix();
glm::vec3 getCameraWorldPosition();
void getCameraFrame(glm::vec3& lookDir, glm::vec3& upDir, glm::vec3& rightDir);
glm::vec3 getLookVec();  // vector giving the "look" direction of the camera frame
glm::vec3 getUpVec();    // vector giving the "up" direction for the scene (unrelated to current camera view)
glm::vec3 getFrontVec(); // vector giving the "front" direction for the scene (unrelated to current camera view)
float getVerticalFieldOfViewDegrees();
float getAspectRatioWidthOverHeight();
ViewRelativeMode getViewRelativeMode();
void setViewRelativeMode(ViewRelativeMode newMode);

// Clip planes
void setClipPlanes(float newNearClip, float newFarClip);
std::tuple<float, float> getClipPlanes();

// Set the camera extrinsics to look at a particular location
void setViewToCamera(const CameraParameters& p);
void lookAt(glm::vec3 cameraLocation, glm::vec3 target, bool flyTo = false);
void lookAt(glm::vec3 cameraLocation, glm::vec3 target, glm::vec3 upDir, bool flyTo = false);
void setVerticalFieldOfViewDegrees(float newVal);

// The "home" view looks at the center of the scene's bounding box.
glm::mat4 computeHomeView();
void resetCameraToHomeView();
void flyToHomeView();
void setViewCenterAndLookAt(glm::vec3 newCenter, bool flyTo = false);
void setViewCenterAndProject(glm::vec3 newCenter);
void setViewCenterRaw(glm::vec3 newCenter);
glm::vec3 getViewCenter();

// These both set the new value, and project the current view as-needed to conform to the new setting
void updateViewAndChangeNavigationStyle(NavigateStyle newStyle, bool flyTo = false);
void updateViewAndChangeUpDir(UpDir newUpDir, bool flyTo = false);
void updateViewAndChangeFrontDir(FrontDir newFrontDir, bool flyTo = false);

// Move the camera with a 'flight' where the camera's position is briefly animated
void startFlightTo(const CameraParameters& p, float flightLengthInSeconds = .4);
void startFlightTo(const glm::mat4& T, float targetFov, float flightLengthInSeconds = .4);
void immediatelyEndFlight();


// == Properties of the view/window

// If using viewports, whenever not specified, all functions apply to the current
// active viewport.

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

// If using viewports, whenever not specified, all functions apply to the current
// active viewport.

// Get world geometry corresponding to a screen pixel (e.g. from a mouse click)
glm::vec3 screenCoordsToWorldRay(glm::vec2 screenCoords, int32_t viewportInd = -1);
glm::vec3 bufferIndsToWorldRay(glm::ivec2 bufferInds, int32_t viewportInd = -1);
glm::vec3 screenCoordsAndDepthToWorldPosition(glm::vec2 screenCoords, float clipDepth, int32_t viewportInd = -1);

// Get and set camera from json string
std::string getViewAsJson();
void setViewFromJson(std::string jsonData, bool flyTo);
std::string getCameraJson(); // DEPRACTED: old names for avove
void setCameraFromJson(std::string jsonData, bool flyTo);
// TODO add functions to serialize all viewports at once

// == Misc helpers

// === Coordinate system conventions ===
//
// Two coordinate systems are used throughout, and MUST NOT be mixed:
//
//   Screen coords  (glm::vec2, float)   — logical pixels, y=0 at TOP,    y increases downward.
//                                         Used by ImGui (io.MousePos), window/widget layout.
//
//   Buffer indices (glm::ivec2, int)    — physical pixels, y=0 at BOTTOM, y increases upward.
//                                         Used by OpenGL framebuffers, ViewParameters::viewportPosY.
//
// High-DPI (e.g. MacOS Retina) machines have a scale factor >1: each 1.0 screen-coord unit spans multiple buffer
// pixels. ViewParameters::viewportRelativePosY and viewportPosY are in BUFFER (y-from-bottom) convention. All functions
// below state which convention their arguments and return values use.

// Convert between buffer indices and screen coords (see definition above) for the full window.
// When using viewports, these operate on global window coordinates, not viewport-local coordinates.
glm::vec2 bufferIndsToScreenCoords(int xPos, int yPos); // buffer → screen
glm::vec2 bufferIndsToScreenCoords(glm::ivec2 bufferInds);
glm::ivec2 screenCoordsToBufferIndsVec(glm::vec2 screenCoords); // screen → buffer
std::tuple<int, int> screenCoordsToBufferInds(glm::vec2 screenCoords);

// Viewport-local versions of the above. Buffer indices and screen coords here are relative to the
// viewport's own origin (not the global window). Pass viewportInd=-1 to use the current active viewport.
// See note above for definition: buffer indices and screen coordinates have differnt scales and origins.
glm::vec2 bufferIndsToScreenCoordsViewport(int xPos, int yPos, int32_t viewportInd = -1);
glm::vec2 bufferIndsToScreenCoordsViewport(glm::ivec2 bufferInds, int32_t viewportInd = -1);
glm::ivec2 screenCoordsToBufferIndsVecViewport(glm::vec2 screenCoords, int32_t viewportInd = -1);
std::tuple<int, int> screenCoordsToBufferIndsViewport(glm::vec2 screenCoords, int32_t viewportInd = -1);

// Given screen coordinates / buffer inds on the whole window, determine which viewport they lie in.
int32_t screenCoordsToViewportInd(glm::vec2 screenCoords);
int32_t bufferIndsToViewportInd(glm::ivec2 bufferInds);

// Given screen coordinates / buffer inds on the whole window, determine which viewport they lie in and the local
// coordinates within that viewport. The return tuple is <localCoords, viewportIndex>
std::tuple<glm::vec2, int32_t> screenCoordsGlobalToViewport(glm::vec2 screenCoords);
std::tuple<glm::ivec2, int32_t> bufferIndsGlobalToViewport(glm::ivec2 bufferInds);

// Given screen coordinates / buffer inds within a viewport, return the corresponding coordinates in whole window.
glm::vec2 screenCoordsViewportToGlobal(glm::vec2 screenCoords, int32_t iViewport);
glm::ivec2 bufferIndsViewportToGlobal(glm::ivec2 bufferInds, int32_t iViewport);


// == Viewport helpers

// High-level helper to create a grid of viewports and arrange them in a grid
void createViewportGrid(int gridWidth, int gridHeight);

// Manually create viewports and arrange them
void setNumViewports(int count);
void setActiveViewportRelativeLocation(float posX, float posY); // on [0,1]
void setActiveViewportRelativeSize(float width, float height);  // on [0,1]

int getNumViewports();

// Set the active viewport
void activateViewport(int viewportInd);
int getActiveViewportIndex();

// Get info about the current active viewport
int getActiveViewportBufferWidth();
int getActiveViewportBufferHeight();

// Returns {x, y, width, height} of a viewport in screen coordinates (see definition above).
// Pass viewportInd=-1 to use the active viewport.
glm::vec4 getViewportImGuiRect(int32_t viewportInd = -1);

// Get a reference to the view parameters for a specific viewport, -1 means the active viewport.
const view::ViewParameters& getViewportParameters(int32_t iViewport = -1);

// If true, all viewports share the same camera and rotation/translation/zoom etc applies
// to all views simultaneously. If false, each viewport has its own independent camera and navigation state.
bool getViewportsShareNavigation();
void setViewportsShareNavigation(bool shareNavigation);

// viewport internal helpers
void createInitialViewport();
void clearViewports();
void validateViewports();
void updateViewportDimensions();
void propagateViewUpdatesToViewports();

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
bool viewIsValid();
void invalidateView();
void ensureViewValid();

float computeRelativeMotionScale();

// For some navigation modes, the view and center must be compatible (e.g.) with turntable the view
// must point at the center along the up axis. This function modifies the center as-needed to ensure compatibility.
void projectCenterToBeValidForView();

// Process user inputs which affect the view
void processTranslate(glm::vec2 delta);
void processRotate(glm::vec2 startP, glm::vec2 endP);
void processClipPlaneShift(float amount);
void processZoom(float amount);
void processKeyboardNavigation(ImGuiIO& io);
void processSetCenter(glm::vec2 screenCoords);

std::tuple<float, float> computeClipPlanes();

// deprecated, bad names, see variants above
glm::vec3 bufferCoordsToWorldRay(glm::vec2 bufferCoords, int32_t viewportInd = -1);


} // namespace view
} // namespace polyscope
