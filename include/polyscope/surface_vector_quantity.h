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
  std::vector<Vector3> vectorRoots;
  std::vector<Vector3> vectors;
  float lengthMult; // longest vector will be this fraction of lengthScale (if not ambient)
  float radiusMult; // radius is this fraction of lengthScale
  std::array<float, 3> vectorColor;
  MeshElement definedOn;

  // A ribbon viz that is appropriate for some fields
  RibbonArtist* ribbonArtist = nullptr;
  bool ribbonEnabled = false;

  // The map that takes values to [0,1] for drawing
  AffineRemapper<Vector3> mapper;

  void writeToFile(std::string filename = "");

  // GL things
  void prepare();
  gl::GLProgram* program = nullptr;
};

class SurfaceVertexVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceVertexVectorQuantity(std::string name, VertexData<Vector3>& vectors_, SurfaceMesh* mesh_,
                              VectorType vectorType_ = VectorType::STANDARD);

  VertexData<Vector3> vectorField;

  virtual void buildInfoGUI(VertexPtr v) override;
};


class SurfaceFaceVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceFaceVectorQuantity(std::string name, FaceData<Vector3>& vectors_, SurfaceMesh* mesh_,
                            VectorType vectorType_ = VectorType::STANDARD);

  FaceData<Vector3> vectorField;

  virtual void buildInfoGUI(FacePtr f) override;
};

class SurfaceFaceIntrinsicVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceFaceIntrinsicVectorQuantity(std::string name, FaceData<Complex>& vectors_, SurfaceMesh* mesh_, int nSym = 1,
                                     VectorType vectorType_ = VectorType::STANDARD);

  FaceData<Complex> vectorField;
  int nSym;

  virtual void draw() override;

  void drawSubUI() override;

  void buildInfoGUI(FacePtr f) override;
};

class SurfaceVertexIntrinsicVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceVertexIntrinsicVectorQuantity(std::string name, VertexData<Complex>& vectors_, SurfaceMesh* mesh_,
                                       int nSym = 1, VectorType vectorType_ = VectorType::STANDARD);

  VertexData<Complex> vectorField;
  int nSym;

  virtual void draw() override;

  void drawSubUI() override;

  void buildInfoGUI(VertexPtr v) override;
};

class SurfaceOneFormIntrinsicVectorQuantity : public SurfaceVectorQuantity {
public:
  SurfaceOneFormIntrinsicVectorQuantity(std::string name, EdgeData<double>& oneForm_, SurfaceMesh* mesh_);

  EdgeData<double> oneForm;
  FaceData<Complex> mappedVectorField;

  virtual void draw() override;

  void drawSubUI() override;

  void buildInfoGUI(EdgePtr e) override;
  void buildInfoGUI(FacePtr f) override;
};

} // namespace polyscope
