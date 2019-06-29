#pragma once

#include "polyscope/utilities.h"

#include <array>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>


// Helpers for management colors (but not colorschemes)

namespace polyscope {

// Color conversions
glm::vec3 RGBtoHSV(glm::vec3 rgb);
glm::vec3 HSVtoRGB(glm::vec3 hsv);

// Stateful helper to color things uniquely
glm::vec3 getNextUniqueColor();

} // namespace polyscope
