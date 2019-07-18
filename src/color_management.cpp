// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/color_management.h"

// Use for color conversion scripts
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <iostream>

using std::cout;
using std::endl;

namespace polyscope {

namespace {

// == Color management helpers

// Clamp to [0,1]
float unitClamp(float x) { return std::max(0.0f, std::min(1.0f, x)); }
glm::vec3 unitClamp(glm::vec3 x) { return {unitClamp(x[0]), unitClamp(x[1]), unitClamp(x[2])}; }

// Used to sample colors. Samples a series of most-distant values from a range [0,1]
// offset from a starting value 'start' and wrapped around. index=0 returns start
//
// Example: if start = 0, emits f(0, i) = {0, 1/2, 1/4, 3/4, 1/8, 5/8, 3/8, 7/8, ...}
//          if start = 0.3 emits (0.3 + f(0, i)) % 1
float getIndexedDistinctValue(float start, int index) {
  if (index < 0) {
    return 0.0;
  }

  // Bit shifty magic to evaluate f()
  float val = 0;
  float p = 0.5;
  while (index > 0) {
    if (index % 2 == 1) {
      val += p;
    }
    index = index >> 1;
    p /= 2.0;
  }

  // Apply modular offset
  val = std::fmod(val + start, 1.0);

  return unitClamp(val);
}

// Get an indexed offset color. Inputs and outputs in RGB
glm::vec3 indexOffsetHue(glm::vec3 baseColor, int index) {
  glm::vec3 baseHSV = RGBtoHSV(baseColor);
  float newHue = getIndexedDistinctValue(baseHSV[0], index);
  glm::vec3 outHSV = {newHue, baseHSV[1], baseHSV[2]};
  return HSVtoRGB(outHSV);
}

// Keep track of unique structure colors
const glm::vec3 uniqueColorBase{28. / 255., 99. / 255., 227. / 255.};
int iUniqueColor = 0;


} // namespace

glm::vec3 getNextUniqueColor() { return indexOffsetHue(uniqueColorBase, iUniqueColor++); }

glm::vec3 RGBtoHSV(glm::vec3 rgb) {
  glm::vec3 hsv;
  ImGui::ColorConvertRGBtoHSV(rgb[0], rgb[1], rgb[2], hsv[0], hsv[1], hsv[2]);
  return unitClamp(hsv);
}

glm::vec3 HSVtoRGB(glm::vec3 hsv) {
  glm::vec3 rgb;
  ImGui::ColorConvertHSVtoRGB(hsv[0], hsv[1], hsv[2], rgb[0], rgb[1], rgb[2]);
  return unitClamp(rgb);
}


} // namespace polyscope
