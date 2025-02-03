// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <complex>
#include <tuple>

#include <glm/glm.hpp>

namespace polyscope {

float computeTValAlongLine(glm::vec3 queryP, glm::vec3 lineStart, glm::vec3 lineEnd);


}