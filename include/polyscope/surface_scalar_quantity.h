#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/histogram.h"
#include "polyscope/gl/colormap_sets.h"

namespace polyscope {

class SurfaceScalarQuantity : public SurfaceQuantityThatDrawsFaces {
public:
  SurfaceScalarQuantity(std::string name, SurfaceMesh* mesh_, std::string definedOn, DataType dataType);

  virtual void draw() override;
  virtual void drawUI() override;
  virtual void setProgramValues(gl::GLProgram* program) override;
  virtual void writeToFile(std::string filename = "");

  // === Members
  const DataType dataType;

protected:

  // Affine data maps and limits
  void resetVizRange();
  float vizRangeLow, vizRangeHigh;
  float dataRangeHigh, dataRangeLow;
  Histogram hist;

  // UI internals
  int iColorMap = 0;
  const std::string definedOn;
};

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

class SurfaceScalarVertexQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarVertexQuantity(std::string name, VertexData<double>& values_, SurfaceMesh* mesh_,
                              DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(VertexPtr v) override;
  virtual void writeToFile(std::string filename = "") override;

  // === Members
  VertexData<double> values;
};

// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

class SurfaceScalarFaceQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarFaceQuantity(std::string name, FaceData<double>& values_, SurfaceMesh* mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(FacePtr f) override;

  // === Members
  FaceData<double> values;
};

// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

class SurfaceScalarEdgeQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarEdgeQuantity(std::string name, EdgeData<double>& values_, SurfaceMesh* mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(EdgePtr e) override;


  // === Members
  EdgeData<double> values;
};

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

class SurfaceScalarHalfedgeQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarHalfedgeQuantity(std::string name, HalfedgeData<double>& values_, SurfaceMesh* mesh_,
                                DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildInfoGUI(HalfedgePtr he) override;

  // === Members
  HalfedgeData<double> values;
};

} // namespace polyscope
