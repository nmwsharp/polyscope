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
std::vector<glm::vec3> convert_tetra_to_tri_dummy(const std::vector<glm::vec4>& tetra_data) {
  std::vector<glm::vec3> tri_data(tetra_data.size());
  for (size_t i = 0; i < tetra_data.size(); i++) {
    tri_data[i] = glm::vec3(tetra_data[i]);
  }
  return tri_data;
}

glm::mat4 tetra_maxbasis_to_cone = glm::mat4(
  1.024611997285031526417134473128e-05, 5.311379457600336889688819042021e-02, 1.403566167392598096341771451989e-01, 2.339692167721606763652886229465e-01,
  2.242812871777761639813936200838e-04, 3.866034406628113262449630838091e-01, 4.705599399499938439994650707376e-01, 4.900264687105190808402710445080e-01,
  8.319914273605166776803798711626e-02, 6.207455522878053688629051976022e-01, 5.023677978953242639903464805684e-01, 4.362919654283259340843414975097e-01,
  1.025172264985438008721985170268e+00, 9.346738062565414228988203149129e-02, 7.081496318011284984983433332673e-02, 4.822367006387873883399564078900e-02
);

glm::vec4 tetra_white_LMSQ_raw = glm::vec4(1.10860594, 1.15393017, 1.18409932, 1.20851132);

glm::vec3 tri_white_LMS_raw = glm::vec3(1.01197671, 0.86641569, 0.56282207);

glm::mat3 LMS_to_XYZ = glm::mat3(
  1.94735469, 0.68990272, 0.00000000,
  -1.41445123, 0.34832189, 0.00000000,
  0.36476327, 0.00000000, 1.93485343
);

glm::mat3 XYZ_to_linRGB = glm::mat3(
  3.2406, -0.9689, 0.0557,
  -1.5372, 1.8758, -0.2040,
  -0.4986, 0.0415, 1.0570
);

std::vector<glm::vec3> convert_tetra_to_tri(const std::vector<glm::vec4>& tetra_data) {
  std::vector<glm::vec3> buf(tetra_data.size());

  for (size_t i = 0; i < tetra_data.size(); i++) {
    // Convert RG1G2B max basis to LMSQ [0, 1].
    glm::vec4 LMSQ = tetra_maxbasis_to_cone * tetra_data[i];
    LMSQ = LMSQ / tetra_white_LMSQ_raw;

    // Convert LMSQ [0, 1] to LMS [0, 1] by dropping the Q-values.
    buf[i] = glm::vec3(LMSQ);

    // Convert LMS [0, 1] to LMS raw by scaling.
    buf[i] = tri_white_LMS_raw * buf[i];

    // Convert LMS raw to XYZ.
    buf[i] = LMS_to_XYZ * buf[i];

    // Convert XYZ to linearized RGB.
    buf[i] = XYZ_to_linRGB * buf[i];

    // Convert linearized RGB to sRGB.
    for (size_t ch = 0; ch < 3; ch++) {
      if (buf[i][ch] <= 0.0031308) {
        buf[i][ch] *= 12.92;
      } else {
        buf[i][ch] = 1.055 * pow(buf[i][ch], 1.0 / 2.4) - 0.055;
      }
      buf[i][ch] = std::max(0.0f, std::min(buf[i][ch], 1.0f)); 
    }
  }
  return buf;
}


// Extract a single color channel from a list of color vectors.
std::vector<float> extract_color_channel(const std::vector<glm::vec4>& tetra_data, int ch) {
  std::vector<float> channel(tetra_data.size());
  for (size_t i = 0; i < tetra_data.size(); i++) {
    channel[i] = tetra_data[i][ch];
  }
  return channel;
}

// Get the Q-values from RG1G2B tetracolor data.
std::vector<float> get_Q_values(const std::vector<glm::vec4>& tetra_data) {
  std::vector<float> Q_values(tetra_data.size());

  for (size_t i = 0; i < tetra_data.size(); i++) {
    // Convert RG1G2B max basis to LMSQ [0, 1].
    glm::vec4 LMSQ = tetra_maxbasis_to_cone * tetra_data[i];
    LMSQ = LMSQ / tetra_white_LMSQ_raw;

    // Select just the Q-value.
    Q_values[i] = LMSQ[3];
  }
  return Q_values;
}

} // namespace polyscope
