#include "polyscope/view.h"

#include "polyscope/polyscope.h"

namespace polyscope {
namespace view {

// Storage for state variables
double dist = 1.0;
Vector3 lookAtPoint{0, 0, 0};
Vector3 cameraSpaceTranslate{0, 0, 0};
Vector3 cameraDirection{0, 0, 1};
Vector3 upDirection{0, 1, 0};
int windowWidth = -1;
int windowHeight = -1;
int bufferWidth = -1;
int bufferHeight = -1;
double fov = 65.0;
double nearClipRatio = 0.01;
double farClipRatio = 20.0;

void mouseDragEvent(double oldX, double oldY, double newX, double newY,
                    bool isRotating) {
  if (oldX == newX && oldY == newY) {
    return;
  }

  // Convert to [-1,1] screen coordinates
  oldX = oldX * 2 - 1.0;
  oldY = -(oldY * 2 - 1.0);
  newX = newX * 2 - 1.0;
  newY = -(newY * 2 - 1.0);

  double delX = newX - oldX;
  double delY = newY - oldY;

  // Process a rotation
  if (isRotating) {
    double delTheta = -delX * PI;
    double delPhi = delY * PI;

    Vector3 leftDirection = cross(upDirection, cameraDirection);

    // Rotate corresponding to theta
    // (the 'up' direction would never change, since it's what we're rotating
    // around)
    cameraDirection = cameraDirection.rotate_around(upDirection, delTheta);

    // Rotate corresponding to phi
    cameraDirection = cameraDirection.rotate_around(leftDirection, delPhi);
    upDirection = upDirection.rotate_around(leftDirection, delPhi);

  }
  // Process a translation
  else {
    double movementScale = state::lengthScale * 0.3;
    cameraSpaceTranslate -= movementScale * Vector3{delX, delY, 0};
  }
}

void mouseScrollEvent(double scrollAmount, bool scrollClipPlane) {
  // Adjust the near clipping plane
  if (scrollClipPlane) {
    if (scrollAmount > 0) {
      nearClipRatio -= 0.03 * nearClipRatio;
    } else {
      nearClipRatio += 0.03 * nearClipRatio;
    }

  }
  // Translate the camera forwards and backwards
  else {
    if (scrollAmount > 0) {
      dist -= 0.03 * state::lengthScale;
    } else {
      dist += 0.03 * state::lengthScale;
    }
  }
}

glm::mat4 getViewMatrix() {
  // Map from world coordinates to camera coordinates
  glm::vec3 eye(cameraDirection.x, cameraDirection.y, cameraDirection.z);
  glm::vec3 center(lookAtPoint.x, lookAtPoint.y, lookAtPoint.z);
  glm::vec3 up(upDirection.x, upDirection.y, upDirection.z);
  glm::mat4 rotateToCameraCoordinates = glm::lookAt(center + eye, center, up);

  // Translate in camera space
  // This is used to support the "sliding" in the screen plane by dragging while
  // holding 'shift'
  glm::mat4 translateCamera =
      glm::translate(glm::mat4(1.0),
                     glm::vec3(-cameraSpaceTranslate.x, -cameraSpaceTranslate.y,
                               -dist - cameraSpaceTranslate.z));

  // Compose the operations together
  glm::mat4 view = translateCamera * rotateToCameraCoordinates;

  return view;
}

glm::mat4 getPerspectiveMatrix() {
  // double farClip = farClipRatio * dist;
  double farClip = farClipRatio * state::lengthScale;
  double nearClip = nearClipRatio * state::lengthScale;
  double fovRad = glm::radians(fov);
  double aspectRatio = (float) bufferWidth / bufferHeight;

  return glm::perspective(fovRad, aspectRatio, nearClip, farClip);
}

Vector3 getCameraWorldPosition() {
  // return lookAtPoint + cameraDirection * dist;

  // This will work no matter how the view matrix is constructed...
  glm::mat4 invViewMat = inverse(getViewMatrix());
  return Vector3{invViewMat[3][0], invViewMat[3][1], invViewMat[3][2]};
}

Vector3 getLightWorldPosition() {
  // The light is over the right shoulder of the camera
  Vector3 leftHand = unit(cross(cameraDirection, upDirection));
  Vector3 lightPos =
      lookAtPoint + 10 * (dist + state::lengthScale) * cameraDirection;
  lightPos = lightPos.rotate_around(upDirection, 3.14159 / 8);
  lightPos = lightPos.rotate_around(leftHand, 3.14159 / 8);

  // Add a vertical component to the light so that when the
  // camera is at the horizon, the shadow doesn't shoot off to infinity
  lightPos = (lightPos + Vector3{0., 20., 0.}) / 2.;

  return lightPos;
}

}  // namespace view
}  // namespace polyscope
