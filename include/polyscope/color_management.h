#pragma once

#include "polyscope/utilities.h"

#include <array>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>


// Helpers for management colors (but not colorschemes)

namespace polyscope {

// Utility def for float colors
typedef std::array<float, 3> Color3f;

// Stateful helper to color top-level structures
Color3f getNextStructureColor();
const Color3f structureBaseColorRGB = {{60. / 255., 120. / 255., 227. / 255.}};


// Factory which emits cached collections of colors. Useful for coloring quantities on a structure
// (all colors managed in RGB)
class SubColorManager {
public:
  SubColorManager();
  SubColorManager(Color3f baseColorRGB);

  Color3f baseColor;

  // Return a new sub color relative to the base. A new color is generate for each new key, but if the same key is
  // used again afterward the same color is returned.
  Color3f getNextSubColor(std::string key);

private:
  std::map<std::string, int> assignedColors;
};


// Print colors
inline std::ostream& operator<<(std::ostream& output, const Color3f& c) {
  output << std::setprecision(std::numeric_limits<float>::max_digits10);
  output << "<" << c[0] << ", " << c[1] << ", " << c[2] << ">";
  return output;
}

inline std::string to_string(const Color3f& c) {
  std::stringstream buffer;
  buffer << c;
  return buffer.str();
}

inline std::string to_string_short(const Color3f& c) { return str_printf("<%1.3f, %1.3f, %1.3f>", c[0], c[1], c[2]); }

} // namespace polyscope
