// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <vector>

#include "polyscope/color_management.h"

namespace polyscope {

// Load a new colormap from a (horizontally oriented) image file
void loadColorMap(std::string cmapName, std::string filename);

namespace render {

// Helper to build a ImGUI dropdown to select color maps. Returns true if changed.
bool buildColormapSelector(std::string& cm, std::string fieldname = "##colormap_picker");


// ColorMaps currently available below
//    Sequential:
//      - viridis (CM_VIRIDIS)
//      - blues (CM_BLUES)
//      - reds (CM_REDS)
//
//    Diverging:
//      - coolwarm (CM_COOLWARM)
//      - purple-green (CM_PIYG)
//
//    Other:
//      - spectral (CM_SPECTRAL)
//      - rainbow (CM_RAINBOW)
//      - jet (CM_JET)
//
//    Cyclic:
//      - phase (CM_PHASE)
//
//  Generate more using the generate_colormap_constant.py script in the misc folder of this repo.
//  Should work on any colormap from http://matplotlib.org/examples/color/colormaps_reference.html
//


// Some colors, while we're at it
const glm::vec3 RGB_TEAL = {0., 178. / 255., 178. / 255.};
const glm::vec3 RGB_BLUE = {150. / 255., 154. / 255., 255. / 255.};
const glm::vec3 RGB_SKYBLUE = {152. / 255., 158. / 255., 200. / 255.};
const glm::vec3 RGB_ORANGE = {1., 0.5, 0.};
const glm::vec3 RGB_BLACK = {0., 0., 0.};
const glm::vec3 RGB_WHITE = {1., 1., 1.};
const glm::vec3 RGB_RED = {0.8, 0., 0.};
const glm::vec3 RGB_DARKGRAY = {.2, .2, .2};
const glm::vec3 RGB_LIGHTGRAY = {.8, .8, .8};
const glm::vec3 RGB_DARKRED = {.2, .0, .0};
const glm::vec3 RGB_PINK = {249. / 255., 45. / 255., 94. / 255.};

// Represents a color map
struct ValueColorMap {

  std::string name;

  std::vector<glm::vec3> values;

  // Samples "val" from the colormap, where val is clamped to [0,1].
  // Returns a vector3 of rgb values, each from [0,1]
  glm::vec3 getValue(double val) const {
    if (!std::isfinite(val)) {
      return {0, 0, 0};
    }

    val = glm::clamp(val, 0.0, 1.0);

    // Find the two nearest indices in to the colormap lookup table, then
    // return a linear blend between them.
    double scaledVal = val * (values.size() - 1);
    double lowerVal = std::floor(scaledVal);
    double upperBlendVal = scaledVal - lowerVal;
    unsigned int lowerInd = static_cast<unsigned int>(lowerVal);
    unsigned int upperInd = lowerInd + 1;

    return (float)(1.0 - upperBlendVal) * values[lowerInd] + (float)upperBlendVal * values[upperInd];
  }
};


} // namespace render
} // namespace polyscope
