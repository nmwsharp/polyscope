#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/ribbon_artist.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

// Represents a general vector field associated with a surface mesh, including
// R3 fields in the ambient space and R2 fields embedded in the surface
class SurfaceVectorQuantity : public SurfaceQuantity {
public:
  SurfaceVectorQuantity(std::string name, SurfaceMesh* mesh_, MeshElement definedOn_,
                        VectorType vectorType_ = VectorType::STANDARD);

  virtual ~SurfaceVectorQuantity() override;

  virtual void draw() override;
  virtual void drawUI() override;

  // Allow children to append to the UI
  virtual void drawSubUI();

  // Do work shared between all constructors
  void finishConstructing();

  // === Members
  const VectorType vectorType;
  std::vector<glm::vec3> vectorRoots;
  std::vector<glm::vec3> vectors;
  float lengthMult; // longest vector will be this fraction of lengthScale (if not ambient)
  float radiusMult; // radius is this fraction of lengthScale
  Color3f vectorColor;
  MeshElement definedOn;

  // A ribbon viz that is appropriate for some fields
  RibbonArtist* ribbonArtist = nullptr;
  bool ribbonEnabled = false;

  // The map that takes values to [0,1] for drawing
  AffineRemapper<glm::vec3> mapper;

  void writeToFile(std::string filename = "");

  // GL things
  void prepare();
  gl::GLProgram* program = nullptr;
};

class SurfaceVertexVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceVertexVectorQuantity(std::string name, std::vector<glm::vec3>& vectors_, SurfaceMesh* mesh_,
                              VectorType vectorType_ = VectorType::STANDARD);

  std::vector<glm::vec3> vectorField;

  virtual void buildVertexInfoGUI(size_t vInd) override;
};

template <class T>
void SurfaceMesh::addVertexVectorQuantity(std::string name, const T& vectors, VectorType vectorType) {
  std::shared_ptr<SurfaceVectorQuantity> q = std::make_shared<SurfaceVertexVectorQuantity>(
      name, standardizeVectorArray<glm::vec3, T, 3>(vectors, nVertices, "vertex vector quantity " + name), this,
      vectorType);
  addSurfaceQuantity(q);
}

class SurfaceFaceVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceFaceVectorQuantity(std::string name, std::vector<glm::vec3>& vectors_, SurfaceMesh* mesh_,
                            VectorType vectorType_ = VectorType::STANDARD);

  std::vector<glm::vec3> vectorField;

  virtual void buildFaceInfoGUI(size_t fInd) override;
};

template <class T>
void SurfaceMesh::addFaceVectorQuantity(std::string name, const T& vectors, VectorType vectorType) {
  std::shared_ptr<SurfaceVectorQuantity> q = std::make_shared<SurfaceFaceVectorQuantity>(
      name, standardizeVectorArray<glm::vec3, T, 3>(vectors, nFaces, "face vector quantity " + name), this, vectorType);
  addSurfaceQuantity(q);
}


class SurfaceFaceIntrinsicVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceFaceIntrinsicVectorQuantity(std::string name, std::vector<glm::vec2>& vectors_, SurfaceMesh* mesh_,
                                     int nSym = 1, VectorType vectorType_ = VectorType::STANDARD);

  std::vector<glm::vec2> vectorField;
  int nSym;

  virtual void draw() override;

  void drawSubUI() override;

  void buildFaceInfoGUI(size_t fInd) override;
};


template <class T>
void SurfaceMesh::addFaceIntrinsicVectorQuantity(std::string name, const T& vectors, int nSym, VectorType vectorType) {
  std::shared_ptr<SurfaceVectorQuantity> q = std::make_shared<SurfaceFaceIntrinsicVectorQuantity>(
      name, standardizeVectorArray<glm::vec3, T, 2>(vectors, nFaces, "face intrinsic vector quantity " + name), this,
      nSym, vectorType);
  addSurfaceQuantity(q);
}


class SurfaceVertexIntrinsicVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceVertexIntrinsicVectorQuantity(std::string name, std::vector<glm::vec2>& vectors_, SurfaceMesh* mesh_,
                                       int nSym = 1, VectorType vectorType_ = VectorType::STANDARD);

  std::vector<glm::vec2> vectorField;
  int nSym;

  virtual void draw() override;

  void drawSubUI() override;

  void buildVertexInfoGUI(size_t vInd) override;
};

template <class T>
void SurfaceMesh::addVertexIntrinsicVectorQuantity(std::string name, const T& vectors, int nSym,
                                                   VectorType vectorType) {
  std::shared_ptr<SurfaceVectorQuantity> q = std::make_shared<SurfaceVertexIntrinsicVectorQuantity>(
      name, standardizeVectorArray<glm::vec3, T, 2>(vectors, nFaces, "vertex intrinsic vector quantity " + name), this,
      nSym, vectorType);
  addSurfaceQuantity(q);
}


class SurfaceOneFormIntrinsicVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceOneFormIntrinsicVectorQuantity(std::string name, std::vector<double>& oneForm_, SurfaceMesh* mesh_);

  std::vector<double> oneForm;
  std::vector<glm::vec2> mappedVectorField;

  virtual void draw() override;

  void drawSubUI() override;

  void buildEdgeInfoGUI(size_t eInd) override;
  void buildFaceInfoGUI(size_t fInd) override;
};

template <class T>
void SurfaceMesh::addOneFormIntrinsicVectorQuantity(std::string name, const T& data) {
  std::shared_ptr<SurfaceVectorQuantity> q = std::make_shared<SurfaceOneFormIntrinsicVectorQuantity>(
      name, standardizeArray<double, T>(data, nEdges, "one form intrinsic vector quantity " + name), this);
  addSurfaceQuantity(q);
}

} // namespace polyscope
