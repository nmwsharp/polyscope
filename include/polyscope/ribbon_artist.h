// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/render/engine.h"
#include "polyscope/structure.h"


namespace polyscope {

class RibbonArtist {

public:
  // Input ribbon is a collection of lines; each line is a list of (position, normal) pairs.
  // - normalOffsetFraction is an offset, relative to polyscope::state::lengthScale, along which ribbons are offset in
  // the normal direction.
  RibbonArtist(Structure& parentStructure, const std::vector<std::vector<std::array<glm::vec3, 2>>>& ribbons,
               std::string uniqueName = "", double normalOffsetFraction = 1e-4);

  void draw();
  void buildParametersGUI();

  Structure& parentStructure;

  RibbonArtist* setEnabled(bool newEnabled);
  bool getEnabled();

  RibbonArtist* setWidth(double newVal, bool isRelative = true);
  double getWidth();

  glm::mat4 objectTransform = glm::mat4(1.0);

  std::shared_ptr<render::ShaderProgram> program;

private:
  // Data
  std::vector<std::vector<std::array<glm::vec3, 2>>> ribbons;
  double normalOffsetFraction;

  PersistentValue<bool> enabled;
  PersistentValue<ScaledValue<float>> ribbonWidth;


  void createProgram();
  void deleteProgram();

  std::string cMap = "spectral";
};


} // namespace polyscope
