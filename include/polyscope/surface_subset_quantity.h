#pragma once

#include "polyscope/surface_mesh.h"

namespace polyscope {

// ========================================================
// ==========            Edge Subset             ==========
// ========================================================

class SurfaceEdgeSubsetQuantity : public SurfaceQuantity {
public:
  SurfaceEdgeSubsetQuantity(std::string name, EdgeData<char>& edgeSubset, SurfaceMesh* mesh_);
  ~SurfaceEdgeSubsetQuantity();

  virtual void draw() override;
  virtual void drawUI() override;

  EdgeData<char> edgeSubset;
  int count;

  virtual void buildInfoGUI(EdgePtr e) override;

private:
  float radius = 0.002;
  std::array<float, 3> color;
  gl::GLProgram* program = nullptr;
};


} // namespace polyscope
