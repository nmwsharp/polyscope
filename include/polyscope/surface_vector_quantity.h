#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/ribbon_artist.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

// ==== Common base class

// Represents a general vector field associated with a surface mesh, including
// R3 fields in the ambient space and R2 fields embedded in the surface
class SurfaceVectorQuantity : public SurfaceMeshQuantity {
public:
  SurfaceVectorQuantity(std::string name, SurfaceMesh& mesh_, MeshElement definedOn_,
                        VectorType vectorType_ = VectorType::STANDARD);


  virtual void draw() override;
  virtual void buildCustomUI() override;

  // Allow children to append to the UI
  virtual void drawSubUI();

  // === Members
  const VectorType vectorType;
  std::vector<glm::vec3> vectorRoots;
  std::vector<glm::vec3> vectors;
  float lengthMult; // longest vector will be this fraction of lengthScale (if not ambient)
  float radiusMult; // radius is this fraction of lengthScale
  glm::vec3 vectorColor;
  MeshElement definedOn;

  // A ribbon viz that is appropriate for some fields
  std::unique_ptr<RibbonArtist> ribbonArtist;
  bool ribbonEnabled = false;

  // The map that takes values to [0,1] for drawing
  AffineRemapper<glm::vec3> mapper;

  void writeToFile(std::string filename = "");

  // GL things
  void prepareProgram();
  std::unique_ptr<gl::GLProgram> program;

protected:
  // Set up the mapper for vectors
  void prepareVectorMapper();
};


// ==== R3 vectors at vertices

class SurfaceVertexVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceVertexVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, SurfaceMesh& mesh_,
                              VectorType vectorType_ = VectorType::STANDARD);

  std::vector<glm::vec3> vectorField;

  virtual std::string niceName() override;
  virtual void buildVertexInfoGUI(size_t vInd) override;
};

template <class T>
void SurfaceMesh::addVertexVectorQuantity(std::string name, const T& vectors, VectorType vectorType) {

  validateSize(vectors, vertexDataSize, "vertex vector quantity " + name);

  SurfaceVectorQuantity* q = new SurfaceVertexVectorQuantity(
      name, applyPermutation(standardizeVectorArray<glm::vec3, T, 3>(vectors), vertexPerm), *this, vectorType);
  addQuantity(q);
}


// ==== R3 vectors at faces

class SurfaceFaceVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceFaceVectorQuantity(std::string name, std::vector<glm::vec3> vectors_, SurfaceMesh& mesh_,
                            VectorType vectorType_ = VectorType::STANDARD);

  std::vector<glm::vec3> vectorField;

  virtual std::string niceName() override;
  virtual void buildFaceInfoGUI(size_t fInd) override;
};

template <class T>
void SurfaceMesh::addFaceVectorQuantity(std::string name, const T& vectors, VectorType vectorType) {

  validateSize(vectors, faceDataSize, "face vector quantity " + name);

  SurfaceVectorQuantity* q = new SurfaceFaceVectorQuantity(
      name, applyPermutation(standardizeVectorArray<glm::vec3, T, 3>(vectors), facePerm), *this, vectorType);
  addQuantity(q);
}


// ==== Intrinsic vectors at faces

class SurfaceFaceIntrinsicVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceFaceIntrinsicVectorQuantity(std::string name, std::vector<glm::vec2> vectors_, SurfaceMesh& mesh_,
                                     int nSym = 1, VectorType vectorType_ = VectorType::STANDARD);

  int nSym;
  std::vector<glm::vec2> vectorField;

  virtual void draw() override;

  void drawSubUI() override;

  virtual std::string niceName() override;
  void buildFaceInfoGUI(size_t fInd) override;
};


template <class T>
void SurfaceMesh::addFaceIntrinsicVectorQuantity(std::string name, const T& vectors, int nSym, VectorType vectorType) {

  validateSize(vectors, faceDataSize, "face intrinsic vector quantity " + name);

  SurfaceVectorQuantity* q = new SurfaceFaceIntrinsicVectorQuantity(
      name, applyPermutation(standardizeVectorArray<glm::vec2, T, 2>(vectors), facePerm), *this, nSym, vectorType);
  addQuantity(q);
}


// ==== Intrinsic vectors at vertices

class SurfaceVertexIntrinsicVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceVertexIntrinsicVectorQuantity(std::string name, std::vector<glm::vec2> vectors_, SurfaceMesh& mesh_,
                                       int nSym = 1, VectorType vectorType_ = VectorType::STANDARD);

  int nSym;
  std::vector<glm::vec2> vectorField;

  virtual void draw() override;

  void drawSubUI() override;

  virtual std::string niceName() override;
  void buildVertexInfoGUI(size_t vInd) override;
};

template <class T>
void SurfaceMesh::addVertexIntrinsicVectorQuantity(std::string name, const T& vectors, int nSym,
                                                   VectorType vectorType) {

  validateSize(vectors, vertexDataSize, "vertex intrinsic vector quantity " + name);

  SurfaceVectorQuantity* q = new SurfaceVertexIntrinsicVectorQuantity(
      name, applyPermutation(standardizeVectorArray<glm::vec2, T, 2>(vectors), vertexPerm), *this, nSym, vectorType);
  addQuantity(q);
}


// ==== Intrinsic one form on edges

class SurfaceOneFormIntrinsicVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceOneFormIntrinsicVectorQuantity(std::string name, std::vector<double> oneForm_, std::vector<char> orientations_,
                                        SurfaceMesh& mesh_);

  std::vector<double> oneForm;
  std::vector<glm::vec2> mappedVectorField;

  virtual void draw() override;

  void drawSubUI() override;

  virtual std::string niceName() override;
  void buildEdgeInfoGUI(size_t eInd) override;
  void buildFaceInfoGUI(size_t fInd) override;
};

// Orientations is `true` if the canonical orientation of the edge points from the lower-indexed vertex to the
// higher-indexed vertex, and `false` otherwise.
template <class T, class O>
void SurfaceMesh::addOneFormIntrinsicVectorQuantity(std::string name, const T& data, const O& orientations) {

  validateSize(data, edgeDataSize, "one form intrinsic vector quantity " + name);

  SurfaceVectorQuantity* q = new SurfaceOneFormIntrinsicVectorQuantity(
      name, applyPermutation(standardizeArray<double, T>(data), edgePerm),
      applyPermutation(standardizeArray<char, O>(orientations), edgePerm), *this);
  addQuantity(q);
}

} // namespace polyscope
