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
    extern double dist;
    extern Vector3 lookAtPoint;
    extern Vector3 cameraSpaceTranslate;
    extern Vector3 cameraDirection;
    extern Vector3 upDirection;
    extern int bufferWidth;
    extern int bufferHeight;
    extern int windowWidth;
    extern int windowHeight;
    extern double fov;
    extern double nearClipRatio;
    extern double farClipRatio;
    extern std::array<float, 4> bgColor; 

    // === View methods

    void processMouseDrag(Vector2 deltaDrag, bool isRotating);
    void processMouseScroll(double scrollAmount, bool scrollClipPlane);
    void setWindowSize(int width, int height);
    void setViewToCamera(const CameraParameters& p);
    void resetCameraToDefault();
    
    glm::mat4 getViewMatrix();
    glm::mat4 getPerspectiveMatrix();
    Vector3 getCameraWorldPosition();
    Vector3 getLightWorldPosition();



} // namespace view    
} // namespace polyscope