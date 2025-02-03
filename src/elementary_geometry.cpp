// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/elementary_geometry.h"


#include <cmath>
#include <vector>

#include <glm/gtx/norm.hpp>

namespace polyscope {

float computeTValAlongLine(glm::vec3 queryP, glm::vec3 lineStart, glm::vec3 lineEnd) {
  glm::vec3 lineVec = lineEnd - lineStart;
  glm::vec3 queryVec = queryP - lineStart;
  float len2 = glm::length2(lineVec);
  float t = glm::dot(queryVec, lineVec) / len2;
  t = glm::clamp(t, 0.f, 1.f);
  return t;
}

} // namespace polyscope