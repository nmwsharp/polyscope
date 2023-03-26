// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <array>

namespace polyscope {

namespace render {

// === the default colormaps themselves
// (stored in color_maps.cpp)

extern const std::vector<glm::vec3> CM_VIRIDIS;
extern const std::vector<glm::vec3> CM_COOLWARM;
extern const std::vector<glm::vec3> CM_BLUES;
extern const std::vector<glm::vec3> CM_PIYG;
extern const std::vector<glm::vec3> CM_SPECTRAL;
extern const std::vector<glm::vec3> CM_RAINBOW;
extern const std::vector<glm::vec3> CM_JET;
extern const std::vector<glm::vec3> CM_TURBO;
extern const std::vector<glm::vec3> CM_REDS;
extern const std::vector<glm::vec3> CM_PHASE;


} // namespace render
} // namespace polyscope
