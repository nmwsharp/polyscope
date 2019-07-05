#pragma once

#include "polyscope/surface_mesh.h"

#include <tuple>

namespace polyscope {

class SurfaceGraphQuantity : public SurfaceMeshQuantity {
public:
  SurfaceGraphQuantity(std::string name, std::vector<glm::vec3> nodes, std::vector<std::array<size_t, 2>> edges,
                       SurfaceMesh& mesh_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;

  std::vector<glm::vec3> nodes;
  std::vector<std::array<size_t, 2>> edges;

private:
  float radius = 0.002;
  glm::vec3 color;

  std::unique_ptr<gl::GLProgram> pointProgram;
  std::unique_ptr<gl::GLProgram> lineProgram;

  void createPrograms();
  void setUniforms();
};

template <class P, class E>
void SurfaceMesh::addSurfaceGraphQuantity(std::string name, const P& nodes, const E& edges) {

  SurfaceGraphQuantity* q = new SurfaceGraphQuantity(name, standardizeVectorArray<glm::vec3, P, 3>(nodes),
                                                     standardizeVectorArray<std::array<size_t, 2>, E, 2>(edges), *this);

  addQuantity(q);
}


} // namespace polyscope
