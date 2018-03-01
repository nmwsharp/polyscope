#pragma once

#include "polyscope/surface_mesh.h"


namespace polyscope {

class RibbonArtist {

public:
  // Input ribbon is a collection of lines; each line is a list of (position, normal) pairs.
  // - normalOffsetFraction is an offset, relative to polyscope::state::lengthScale, along which ribbons are offset in
  // the normal direction.
  RibbonArtist(const std::vector<std::vector<std::array<geometrycentral::Vector3, 2>>>& ribbons,
               double normalOffsetFraction = 1e-4);
  ~RibbonArtist();

  void draw();
  void buildParametersGUI();

  float ribbonWidth = -1;
  bool enabled = true;


  glm::mat4 objectTransform = glm::mat4(1.0);

private:

  // Data
  std::vector<std::vector<std::array<geometrycentral::Vector3, 2>>> ribbons;
  double normalOffsetFraction;

  gl::GLProgram* program = nullptr;

  void createProgram();
  void deleteProgram();
  
  int iColorMap = 5; // spectral
};


} // namespace polyscope
