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
               std::string uniqueName = "ribbon", double normalOffsetFraction = 1e-4);

  void draw();
  void buildParametersGUI();

  Structure& parentStructure;
  const std::string uniqueName;

  RibbonArtist* setEnabled(bool newEnabled);
  bool getEnabled();

  RibbonArtist* setWidth(double newVal, bool isRelative = true);
  double getWidth();

  RibbonArtist* setMaterial(std::string mat);
  std::string getMaterial();

private:
  // Data
  std::vector<std::vector<std::array<glm::vec3, 2>>> ribbons;
  double normalOffsetFraction;

  PersistentValue<bool> enabled;
  PersistentValue<ScaledValue<float>> ribbonWidth;
  PersistentValue<std::string> material;

  void createProgram();
  void deleteProgram();

  std::string cMap = "spectral";
  
  std::shared_ptr<render::ShaderProgram> program;
};


} // namespace polyscope
