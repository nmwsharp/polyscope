#pragma once

#include "polyscope/gl/colormaps.h"


// Lists of colormaps to be chosen from

// Storing colormaps are intionally stored in separate matching lists of colomap/names, because this plays nice with
// ImGUI

namespace polyscope {
namespace gl {

// All of the colormaps we have
// All quantitative colormaps
static const std::vector<const Colormap*> allColormaps {
  &CM_VIRIDIS, 
  &CM_COOLWARM, 
  &CM_PIYG, 
  &CM_BLUES,
  &CM_REDS,
  &CM_SPECTRAL, 
  &CM_RAINBOW, 
  &CM_CONST_RED
};
static const char* allColormapNames[] = {
  "viridis", 
  "coolwarm", 
  "purple-green", 
  "blues",
  "reds",
  "spectral", 
  "rainbow", 
  "constant red",
};


// All quantitative colormaps
static const std::vector<const Colormap*> quantitativeColormaps {
  &CM_VIRIDIS, 
  &CM_COOLWARM, 
  &CM_PIYG, 
  &CM_BLUES,
  &CM_REDS
};
static const char* quantitativeColormapNames[] = {
  "viridis", 
  "coolwarm", 
  "purple-green", 
  "blues",
  "reds"
};


// Quantitative colormaps which diverge away from the center
static const std::vector<const Colormap*> divergingColormaps {
  &CM_COOLWARM, 
  &CM_PIYG, 
};
static const char* divergingColormapNames[] = {
  "coolwarm", 
  "purple-green", 
};


// Qualitative colormaps (colors on a line which don't necessarily encode a scale)
static const std::vector<const Colormap*> qualitativeColormaps {
  &CM_SPECTRAL, 
  &CM_RAINBOW, 
};
static const char* qualitativeColormapNames[] = {
  "spectral", 
  "rainbow", 
};




} // namespace gl
} // namespace polyscope
