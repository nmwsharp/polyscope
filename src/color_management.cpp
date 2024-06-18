// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/color_management.h"
#include "polyscope/colors.h"

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
// (We also use this logic via a duplicate implementation in some shaders)
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

// Custom access function for a vector of Tricolor objects.
std::vector<glm::vec3>
adaptorF_custom_convertArrayOfVectorToStdVector(const std::vector<Tricolor>& inputData) {
  std::vector<glm::vec3> out;
  for (auto v : inputData) {
    out.push_back(glm::vec3(v[0], v[1], v[2]));
  }
  return out;
}

// Custom access function for a vector of Tetracolor objects.
std::vector<glm::vec4>
adaptorF_custom_convertArrayOfVectorToStdVector(const std::vector<Tetracolor>& inputData) {
  std::vector<glm::vec4> out;
  for (auto v : inputData) {
    out.push_back(glm::vec4(v[0], v[1], v[2], v[3]));
  }
  return out;
}

// Convert from RG1G2B to RGB.
// Dummy function at the moment.
std::vector<glm::vec3> convert_tetra_to_tri(const std::vector<glm::vec4>& tetra_data) {
  std::vector<glm::vec3> tri_data(tetra_data.size());
  for (size_t i = 0; i < tetra_data.size(); i++) {
    tri_data[i] = glm::vec3(tetra_data[i]);
  }
  return tri_data;
}

// Extract a single color channel from a list of color vectors.
std::vector<float> extract_color_channel(const std::vector<glm::vec4>& colors, int ch) {
  std::vector<float> channel(colors.size());
  for (size_t i = 0; i < colors.size(); i++) {
    channel[i] = colors[i][ch];
  }
  return channel;
}

// Get the Q-values from RG1G2B tetracolor data.
std::vector<float> get_Q_values(const std::vector<glm::vec4>& tetracolors) {
  // TODO: dummy implementation
  return extract_color_channel(tetracolors, 0);
}

} // namespace polyscope
