#pragma once

#include <array>

#include "polyscope/gl/gl_utils.h"
#include "polyscope/camera_parameters.h"

// GLM for view matrices
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace polyscope {
namespace view {

    // === View state
    extern int bufferWidth;
    extern int bufferHeight;
    extern int windowWidth;
    extern int windowHeight;
    extern double nearClipRatio;
    extern double farClipRatio;
    extern std::array<float, 4> bgColor; 

    // Current view camera parameters
    extern glm::mat4x4 viewMat;
    extern double fov; // in the y direction

    // "Flying" view
    extern bool midflight;
    extern float flightStartTime;
    extern float flightEndTime;
    extern glm::mat4x4 flightTargetView, flightInitialView;
    extern float flightTargetFov, flightInitialFov;

    // === View methods

    void processMouseDrag(Vector2 deltaDrag, bool isRotating);
    void processMouseScroll(double scrollAmount, bool scrollClipPlane);
    void setWindowSize(int width, int height);
    void setViewToCamera(const CameraParameters& p);
    void resetCameraToDefault();
    
    glm::mat4 getCameraViewMatrix();
    glm::mat4 getCameraPerspectiveMatrix();
    Vector3 getCameraWorldPosition();

    void getCameraFrame(Vector3& lookDir, Vector3& upDir, Vector3& rightDir);

    // Flight-related
    void startFlightTo(const CameraParameters& p, float flightLengthInSeconds);
    void immediatelyEndFlight();
    void updateFlight();



} // namespace view    
} // namespace polyscope