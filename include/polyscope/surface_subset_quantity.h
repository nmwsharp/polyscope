// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/surface_mesh.h"

namespace polyscope {

// ========================================================
// ==========            Edge Subset             ==========
// ========================================================

class SurfaceEdgeSubsetQuantity : public SurfaceMeshQuantity {
public:
  SurfaceEdgeSubsetQuantity(std::string name, std::vector<char> edgeSubset, SurfaceMesh* mesh_);
  ~SurfaceEdgeSubsetQuantity();

  virtual void draw() override;
  virtual void drawUI() override;

  std::vector<char> edgeSubset;
  int count;

  virtual void buildEdgeInfoGUI(size_t eInd) override;

private:
  float radius = 0.002;
  Color3f color;
  gl::GLProgram* program = nullptr;
};

template <class T>
void SurfaceMesh::addEdgeSubsetQuantity(std::string name, const T& subset) {

  validateSize(subset, edgeDataSize, "edge subset quantity " + name);

  std::shared_ptr<SurfaceEdgeSubsetQuantity> q = std::make_shared<SurfaceEdgeSubsetQuantity>(
      name, applyPermutation(standardizeArray<char, T>(subset), edgePerm), this);
  addSurfaceQuantity(q);
}


} // namespace polyscope
