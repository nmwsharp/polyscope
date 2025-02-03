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

glm::vec3 projectToPlane(glm::vec3 queryP, glm::vec3 planeNormal, glm::vec3 pointOnPlane) {
  glm::vec3 pVec = queryP - pointOnPlane;
  glm::vec3 pVecOrtho = glm::dot(pVec, planeNormal) * planeNormal;
  return queryP - pVecOrtho;
}

float signedTriangleArea(glm::vec3 normal, glm::vec3 pA, glm::vec3 pB, glm::vec3 pC) {
  glm::vec3 cross = glm::cross(pB - pA, pC - pA);
  float sign = glm::sign(glm::dot(normal, cross));
  float area = glm::length(cross) / 2.;
  return sign * area;
}

} // namespace polyscope