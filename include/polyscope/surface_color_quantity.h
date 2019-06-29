#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceColorQuantity : public SurfaceMeshQuantity {
public:
  SurfaceColorQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn);

  virtual void draw() override;
  virtual std::string niceName() override;

  virtual void geometryChanged() override;

protected:
  // UI internals
  const std::string definedOn;
  std::unique_ptr<gl::GLProgram> program;

  // Helpers
  virtual void createProgram() = 0;
};

// ========================================================
// ==========           Vertex Color             ==========
// ========================================================

class SurfaceColorVertexQuantity : public SurfaceColorQuantity {
public:
  SurfaceColorVertexQuantity(std::string name, std::vector<glm::vec3> values_, SurfaceMesh& mesh_);

  virtual void createProgram() override;
  void fillColorBuffers(gl::GLProgram& p);

  void buildVertexInfoGUI(size_t vInd) override;

  // === Members
  std::vector<glm::vec3> values;
};

template <class T>
void SurfaceMesh::addVertexColorQuantity(std::string name, const T& colors) {

  validateSize<T>(colors, vertexDataSize, "vertex color quantity " + name);

  SurfaceColorQuantity* q = new SurfaceColorVertexQuantity(
      name, applyPermutation(standardizeVectorArray<glm::vec3, T, 3>(colors), vertexPerm), *this);
  addQuantity(q);
}

// ========================================================
// ==========             Face Color             ==========
// ========================================================

class SurfaceColorFaceQuantity : public SurfaceColorQuantity {
public:
  SurfaceColorFaceQuantity(std::string name, std::vector<glm::vec3> values_, SurfaceMesh& mesh_);

  virtual void createProgram() override;
  void fillColorBuffers(gl::GLProgram& p);

  void buildFaceInfoGUI(size_t fInd) override;

  // === Members
  std::vector<glm::vec3> values;
};

template <class T>
void SurfaceMesh::addFaceColorQuantity(std::string name, const T& colors) {

  validateSize<T>(colors, faceDataSize, "face color quantity " + name);

  SurfaceColorQuantity* q = new SurfaceColorFaceQuantity(
      name, applyPermutation(standardizeVectorArray<glm::vec3, T, 3>(colors), facePerm), *this);
  addQuantity(q);
}


} // namespace polyscope
