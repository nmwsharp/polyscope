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
  SurfaceColorVertexQuantity(std::string name, VertexData<Vector3>& values_, SurfaceMesh* mesh_);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(VertexPtr v) override;

  // === Members
  VertexData<Vector3> values;
};

// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

class SurfaceColorFaceQuantity : public SurfaceColorQuantity {
public:
  SurfaceColorFaceQuantity(std::string name, FaceData<Vector3>& values_, SurfaceMesh* mesh_);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(FacePtr f) override;

  // === Members
  FaceData<Vector3> values;
};

/*
// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

class SurfaceScalarEdgeQuantity : public SurfaceColorQuantity {
 public:
  SurfaceScalarEdgeQuantity(std::string name, EdgeData<double>& values_,
                            SurfaceMesh* mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  // === Members
  EdgeData<double> values;
};

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

class SurfaceScalarHalfedgeQuantity : public SurfaceColorQuantity {
 public:
  SurfaceScalarHalfedgeQuantity(std::string name, HalfedgeData<double>& values_,
                                SurfaceMesh* mesh_,
                                DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  // === Members
  HalfedgeData<double> values;
};
*/

} // namespace polyscope