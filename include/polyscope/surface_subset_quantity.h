#pragma once

#include "polyscope/surface_mesh.h"

namespace polyscope {

// ========================================================
// ==========            Edge Subset             ==========
// ========================================================

class SurfaceEdgeSubsetQuantity : public SurfaceQuantity {
public:
  SurfaceEdgeSubsetQuantity(std::string name, std::vector<char>& edgeSubset, SurfaceMesh* mesh_);
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
void SurfaceMesh::addSubsetQuantity(std::string name, const T& subset) {
  std::shared_ptr<SurfaceEdgeSubsetQuantity> q = std::make_shared<SurfaceEdgeSubsetQuantity>(
      name, standardizeArray<glm::vec3, T>(subset, nEdges, "edge subset quantity " + name), this);
  addSurfaceQuantity(q);
}



} // namespace polyscope
