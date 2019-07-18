// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <vector>

#include "polyscope/color_management.h"

namespace polyscope {
namespace gl {

#define COLORMAP_DATA_LENGTH 500


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

// All of the color maps
enum class ColorMapID { VIRIDIS = 0, COOLWARM, BLUES, REDS, PIYG, PHASE, SPECTRAL, RAINBOW, JET };

static std::vector<ColorMapID> allColorMaps{ColorMapID::VIRIDIS,  ColorMapID::COOLWARM, ColorMapID::BLUES,
                                            ColorMapID::REDS,     ColorMapID::PIYG,     ColorMapID::PHASE,
                                            ColorMapID::SPECTRAL, ColorMapID::RAINBOW,  ColorMapID::JET};

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
    double scaledVal = val * (COLORMAP_DATA_LENGTH - 1);
    double lowerVal = std::floor(scaledVal);
    double upperBlendVal = scaledVal - lowerVal;
    unsigned int lowerInd = static_cast<unsigned int>(lowerVal);
    unsigned int upperInd = lowerInd + 1;

    return (float)(1.0 - upperBlendVal) * values[lowerInd] + (float)upperBlendVal * values[upperInd];
  }
};


inline std::string colorMapName(ColorMapID cmap) {
  switch (cmap) {
  case ColorMapID::VIRIDIS:
    return "viridis";
    break;
  case ColorMapID::COOLWARM:
    return "coolwarm";
    break;
  case ColorMapID::PIYG:
    return "pink-green";
    break;
  case ColorMapID::BLUES:
    return "blues";
    break;
  case ColorMapID::REDS:
    return "reds";
    break;
  case ColorMapID::SPECTRAL:
    return "spectral";
    break;
  case ColorMapID::RAINBOW:
    return "rainbow";
    break;
  case ColorMapID::JET:
    return "jet";
    break;
  case ColorMapID::PHASE:
    return "phase";
    break;
  }
  return "SOMETHING_IS_WRONG";
}

// Helper to build a ImGUI dropdown to select color maps. Returns true if changed.
bool buildColormapSelector(ColorMapID& cm, std::string fieldname = "##colormap_picker");

// === the colormaps themselves
// (stored in color_maps.cpp)

extern const ValueColorMap CM_VIRIDIS;
extern const ValueColorMap CM_COOLWARM;
extern const ValueColorMap CM_BLUES;
extern const ValueColorMap CM_PIYG;
extern const ValueColorMap CM_SPECTRAL;
extern const ValueColorMap CM_RAINBOW;
extern const ValueColorMap CM_JET;
extern const ValueColorMap CM_REDS;
extern const ValueColorMap CM_PHASE;


inline const ValueColorMap& getColorMap(ColorMapID cmap) {
  switch (cmap) {
  case ColorMapID::VIRIDIS:
    return CM_VIRIDIS;
    break;
  case ColorMapID::COOLWARM:
    return CM_COOLWARM;
    break;
  case ColorMapID::PIYG:
    return CM_PIYG;
    break;
  case ColorMapID::BLUES:
    return CM_BLUES;
    break;
  case ColorMapID::REDS:
    return CM_REDS;
    break;
  case ColorMapID::SPECTRAL:
    return CM_SPECTRAL;
    break;
  case ColorMapID::RAINBOW:
    return CM_RAINBOW;
    break;
  case ColorMapID::JET:
    return CM_JET;
    break;
  case ColorMapID::PHASE:
    return CM_PHASE;
    break;
  }

  throw std::runtime_error("shouldn't happen");
  return CM_VIRIDIS;
}


} // namespace gl
} // namespace polyscope
