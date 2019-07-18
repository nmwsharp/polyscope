// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/color_maps.h"
#include "polyscope/histogram.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceScalarQuantity : public SurfaceMeshQuantity {
public:
  SurfaceScalarQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn, DataType dataType);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;
  virtual void geometryChanged() override;

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
  gl::ColorMapID cMap;
  const std::string definedOn;
  std::unique_ptr<gl::GLProgram> program;

  // Helpers
  virtual void createProgram() = 0;
  void setProgramUniforms(gl::GLProgram& program);
};

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

class SurfaceVertexScalarQuantity : public SurfaceScalarQuantity {
public:
  SurfaceVertexScalarQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                              DataType dataType_ = DataType::STANDARD);

  virtual void createProgram() override;

  void fillColorBuffers(gl::GLProgram& p);

  void buildVertexInfoGUI(size_t vInd) override;
  virtual void writeToFile(std::string filename = "") override;

  // === Members
  std::vector<double> values;
};


// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

class SurfaceFaceScalarQuantity : public SurfaceScalarQuantity {
public:
  SurfaceFaceScalarQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                            DataType dataType_ = DataType::STANDARD);

  virtual void createProgram() override;

  void fillColorBuffers(gl::GLProgram& p);

  void buildFaceInfoGUI(size_t fInd) override;

  // === Members
  std::vector<double> values;
};


// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

class SurfaceEdgeScalarQuantity : public SurfaceScalarQuantity {
public:
  SurfaceEdgeScalarQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceVertexScalarQuantity();

  virtual void createProgram() override;

  void fillColorBuffers(gl::GLProgram& p);

  void buildEdgeInfoGUI(size_t edgeInd) override;


  // === Members
  std::vector<double> values;
};

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

class SurfaceHalfedgeScalarQuantity : public SurfaceScalarQuantity {
public:
  SurfaceHalfedgeScalarQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                                DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceVertexScalarQuantity();

  virtual void createProgram() override;

  void fillColorBuffers(gl::GLProgram& p);

  void buildHalfedgeInfoGUI(size_t heInd) override;

  // === Members
  std::vector<double> values;
};


} // namespace polyscope
