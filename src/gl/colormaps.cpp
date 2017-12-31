#include "polyscope/gl/colormaps.h"

#include <cmath>

namespace polyscope {
namespace gl {

Vector3 Colormap::getValue(double val) const {

  // Return black if the input is NaN or inf
  // alternately, could throw and error here
  if (!std::isfinite(val)) {
    return Vector3{0, 0, 0};
  }

  val = clamp(val, 0.0, 1.0);

  // Find the two nearest indices in to the colormap lookup table, then
  // return a linear blend between them.
  double scaledVal = val * (COLORMAP_DATA_LENGTH - 1);
  double lowerVal = std::floor(scaledVal);
  double upperBlendVal = scaledVal - lowerVal;
  unsigned int lowerInd = static_cast<unsigned int>(lowerVal);
  unsigned int upperInd = lowerInd + 1;

  return (1.0 - upperBlendVal) * values[lowerInd] + upperBlendVal * values[upperInd];
}

} // namespace gl
} // namespace polyscope