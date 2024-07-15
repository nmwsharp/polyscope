// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

namespace polyscope {

extern glm::mat4 tetra_maxbasis_to_cone;

class Tricolor {
public:
  float r;
  float g;
  float b;

  Tricolor(float r_ = 0, float g_ = 0, float b_ = 0) {
    r = r_;
    g = g_;
    b = b_;
  }

  // Array-style access.
  float& operator[](int k) {
    return (&r)[k];
  }

}; // class Tricolor


class Tetracolor {
public:
  float r;
  float g1;
  float g2;
  float b;

  Tetracolor(float r_ = 0, float g1_ = 0, float g2_ = 0, float b_ = 0) {
    r = r_;
    g1 = g1_;
    g2 = g2_;
    b = b_;
  }

  Tetracolor(glm::vec4 tetracolor_vec) {
    r = tetracolor_vec[0];
    g1 = tetracolor_vec[1];
    g2 = tetracolor_vec[2];
    b = tetracolor_vec[3];
  }

  // Array-style access.
  float& operator[](int k) {
    return (&r)[k];
  }

}; // class Tetracolor

std::vector<glm::vec3> adaptorF_custom_convertArrayOfVectorToStdVector(const std::vector<Tricolor>& inputData);

std::vector<glm::vec4> adaptorF_custom_convertArrayOfVectorToStdVector(const std::vector<Tetracolor>& inputData);

std::vector<glm::vec3> convert_tetra_to_tri_dummy(const std::vector<glm::vec4>& tetra_data);

std::vector<glm::vec3> convert_tetra_to_tri(const std::vector<glm::vec4>& tetra_data);

// TODO: might want to template this function
std::vector<float> extract_color_channel(const std::vector<glm::vec4>& tetra_data, int ch);

std::vector<float> get_Q_values(const std::vector<glm::vec4>& tetra_data);

} // namespace polyscope
