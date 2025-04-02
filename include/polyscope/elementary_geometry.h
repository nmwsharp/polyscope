// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <complex>
#include <tuple>

#include <glm/glm.hpp>

namespace polyscope {

// Compute t \in [0,1] for a point along hte line from lineStart -- lineEnd
float computeTValAlongLine(glm::vec3 queryP, glm::vec3 lineStart, glm::vec3 lineEnd);

// Project a point onto a plane. planeNormal must be unit
glm::vec3 projectToPlane(glm::vec3 queryP, glm::vec3 planeNormal, glm::vec3 pointOnPlane);

// Compute the signed area of triangle ABC which lies in the plane give by normal
float signedTriangleArea(glm::vec3 normal, glm::vec3 pA, glm::vec3 pB, glm::vec3 pC);

} // namespace polyscope