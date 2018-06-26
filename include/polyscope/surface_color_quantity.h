#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceColorQuantity : public SurfaceQuantityThatDrawsFaces {
public:
  SurfaceColorQuantity(std::string name, SurfaceMesh* mesh_, std::string definedOn);

  virtual void draw() override;
  virtual void drawUI() override;

  // UI internals
  const std::string definedOn;
};

// ========================================================
// ==========           Vertex Color             ==========
// ========================================================

class SurfaceColorVertexQuantity : public SurfaceColorQuantity {
public:
  SurfaceColorVertexQuantity(std::string name, std::vector<Color3f> values_, SurfaceMesh* mesh_);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildVertexInfoGUI(size_t vInd) override;

  // === Members
  std::vector<Color3f> values;
};

template <class T>
void SurfaceMesh::addVertexColorQuantity(std::string name, const T& colors) {
  std::shared_ptr<SurfaceColorQuantity> q = std::make_shared<SurfaceColorVertexQuantity>(
      name, standardizeArray<glm::vec3, T>(colors, triMesh.nVertices(), "vertex color quantity " + name), this);
  addSurfaceQuantity(q);
}

// ========================================================
// ==========             Face Color             ==========
// ========================================================

class SurfaceColorFaceQuantity : public SurfaceColorQuantity {
public:
  SurfaceColorFaceQuantity(std::string name, std::vector<Color3f> values_, SurfaceMesh* mesh_);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildFaceInfoGUI(size_t fInd) override;

  // === Members
  std::vector<Color3f> values;
};

template <class T>
void SurfaceMesh::addFaceColorQuantity(std::string name, const T& colors) {
  std::shared_ptr<SurfaceColorQuantity> q = std::make_shared<SurfaceColorFaceQuantity>(
      name, standardizeArray<glm::vec3, T>(colors, triMesh.nFaces(), "face color quantity " + name), this);
  addSurfaceQuantity(q);
}



} // namespace polyscope
