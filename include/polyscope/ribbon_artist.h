// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/gl_utils.h"
#include "polyscope/structure.h"


namespace polyscope {

class RibbonArtist {

public:
  // Input ribbon is a collection of lines; each line is a list of (position, normal) pairs.
  // - normalOffsetFraction is an offset, relative to polyscope::state::lengthScale, along which ribbons are offset in
  // the normal direction.
  RibbonArtist(Structure& parentStructure, const std::vector<std::vector<std::array<glm::vec3, 2>>>& ribbons,
               double normalOffsetFraction = 1e-4);

  void draw();
  void buildParametersGUI();

  Structure& parentStructure;

  float ribbonWidth = -1;
  bool enabled = true;


  glm::mat4 objectTransform = glm::mat4(1.0);

private:
  // Data
  std::vector<std::vector<std::array<glm::vec3, 2>>> ribbons;
  double normalOffsetFraction;

  std::unique_ptr<gl::GLProgram> program;

  void createProgram();
  void deleteProgram();

  gl::ColorMapID cMap = gl::ColorMapID::SPECTRAL;
};


} // namespace polyscope
