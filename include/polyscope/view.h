#pragma once

#include "polyscope/gl/gl_utils.h"

// GLM for view matrices
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

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

    // === View methods
    glm::mat4 getViewMatrix();
    glm::mat4 getPerspectiveMatrix();
    Vector3 getCameraWorldPosition();
    Vector3 getLightWorldPosition();

    void mouseDragEvent(double oldX, double oldY, double newX, double newY, bool isRotating);
    void mouseScrollEvent(double scrollAmount, bool scrollClipPlane);
    void setWindowSize(int width, int height);
    

} // namespace view    
} // namespace polyscope