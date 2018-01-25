#pragma once

#include <array>
#include <map>
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

} // namespace polyscope
