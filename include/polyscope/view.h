#pragma once

#include <array>

#include "polyscope/camera_parameters.h"
#include "polyscope/gl/gl_utils.h"

// GLM for view matrices
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/dual_quaternion.hpp"
#include "glm/gtx/norm.hpp"  // necessary for dual_quaternion below
#undef GLM_ENABLE_EXPERIMENTAL

namespace polyscope {
namespace view {

// === View state
extern int bufferWidth;
extern int bufferHeight;
extern int windowWidth;
extern int windowHeight;
extern int initWindowPosX;
extern int initWindowPosY;
extern double nearClipRatio;
extern double farClipRatio;
extern std::array<float, 4> bgColor;

// Current view camera parameters
extern glm::mat4x4 viewMat;
extern double fov;  // in the y direction

// "Flying" view
extern bool midflight;
extern float flightStartTime;
extern float flightEndTime;
extern glm::dualquat flightTargetViewR, flightInitialViewR;
extern glm::vec3 flightTargetViewT, flightInitialViewT;
extern float flightTargetFov, flightInitialFov;

// === View methods

void processTranslate(Vector2 delta);
void processRotate(float delTheta, float delPhi);

// Arcball roatation. startP and endP should be in [-1,1]
void processRotateArcball(Vector2 startP, Vector2 endP);

void processClipPlaneShift(double amount);
void processZoom(double amount);

void setWindowSize(int width, int height);
void setViewToCamera(const CameraParameters& p);
void resetCameraToDefault();
void flyToDefault();

glm::mat4 getCameraViewMatrix();
glm::mat4 getCameraPerspectiveMatrix();
Vector3 getCameraWorldPosition();

void getCameraFrame(Vector3& lookDir, Vector3& upDir, Vector3& rightDir);

// Flight-related
void startFlightTo(const CameraParameters& p, float flightLengthInSeconds=.25);
void startFlightTo(const glm::mat4& T, float targetFov, float flightLengthInSeconds=.25);
void immediatelyEndFlight();
void splitTransform(const glm::mat4& trans, glm::mat3x4& R, glm::vec3& T);
glm::mat4 buildTransform(const glm::mat3x4& R, const glm::vec3& T);
void updateFlight();

}  // namespace view
}  // namespace polyscope
