// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/camera_parameters.h"

#include <cstdio>
#include <iostream>

#include "glm/gtc/matrix_access.hpp"

using std::cout;
using std::endl;

namespace polyscope {

using namespace glm;

// CameraParameters::CameraParameters()
//     : T(0.0), R(glm::mat3x3(1.0)), focalLengths(1.0) {}
// CameraParameters::CameraParameters() : E(1.0), focalLengths(1.0) {}
CameraParameters::CameraParameters() : E(1.0), fov(60.) {}

glm::vec3 CameraParameters::getT() const { return vec3(E[3][0], E[3][1], E[3][2]); }

glm::mat3x3 CameraParameters::getR() const {
  mat3x3 R;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      R[i][j] = E[i][j];
    }
  }
  return R;
}

glm::vec3 CameraParameters::getPosition() const { return -transpose(getR()) * getT(); }

glm::vec3 CameraParameters::getLookDir() const { return normalize(transpose(getR()) * vec3(0.0, 0.0, -1.0)); }

glm::vec3 CameraParameters::getUpDir() const { return normalize(transpose(getR()) * vec3(0.0, 1.0, 0.0)); }

glm::vec3 CameraParameters::getRightDir() const { return normalize(transpose(getR()) * vec3(1.0, 0.0, 0.0)); }

void prettyPrint(glm::mat4x4 M) {
  char buff[50];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      std::snprintf(buff, 50, "%.5f  ", M[j][i]);
      cout << buff;
    }
    cout << endl;
  }
}

} // namespace polyscope