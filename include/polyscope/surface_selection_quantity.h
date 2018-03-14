#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/histogram.h"
#include "polyscope/gl/colormap_sets.h"

namespace polyscope {

class SurfaceSelectionQuantity : public SurfaceQuantityThatDrawsFaces {
public:
  SurfaceSelectionQuantity(std::string name, SurfaceMesh* mesh_, std::string definedOn);

  virtual void draw() override;
  virtual void drawUI() override;
  virtual void userEdit() = 0;

  // === Members

protected:

  // UI internals
  int iColorMap = 0;
  const std::string definedOn;
};

// ========================================================
// ==========           Vertex Selection         ==========
// ========================================================

class SurfaceSelectionVertexQuantity : public SurfaceSelectionQuantity {
public:
  SurfaceSelectionVertexQuantity(std::string name, VertexData<char>& membership_, SurfaceMesh* mesh_);
  //   ~SurfaceSelectionVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(VertexPtr v) override;

  void userEdit() override;
  virtual void setProgramValues(gl::GLProgram* program) override;

  // === Members
  VertexData<char> membership; // 1 if in, 0 otherwise

private:
  void userEditCallback();
  bool membershipStale = false;
};

/*
// ========================================================
// ==========            Face Selection          ==========
// ========================================================

class SurfaceSelectionFaceQuantity : public SurfaceSelectionQuantity {
public:
  SurfaceSelectionFaceQuantity(std::string name, FaceData<double>& values_, SurfaceMesh* mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceSelectionVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(FacePtr f) override;

  // === Members
  FaceData<double> values;
};

// ========================================================
// ==========            Edge Selection          ==========
// ========================================================

class SurfaceSelectionEdgeQuantity : public SurfaceSelectionQuantity {
public:
  SurfaceSelectionEdgeQuantity(std::string name, EdgeData<double>& values_, SurfaceMesh* mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceSelectionVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(EdgePtr e) override;


  // === Members
  EdgeData<double> values;
};

// ========================================================
// ==========          Halfedge Selection        ==========
// ========================================================

class SurfaceSelectionHalfedgeQuantity : public SurfaceSelectionQuantity {
public:
  SurfaceSelectionHalfedgeQuantity(std::string name, HalfedgeData<double>& values_, SurfaceMesh* mesh_,
                                DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceSelectionVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(HalfedgePtr he) override;

  // === Members
  HalfedgeData<double> values;
};
*/
} // namespace polyscope
