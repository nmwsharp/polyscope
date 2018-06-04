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
  SurfaceScalarVertexQuantity(std::string name, std::vector<double> values_, SurfaceMesh* mesh_,
                              DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildVertexInfoGUI(size_t vInd) override;
  virtual void writeToFile(std::string filename = "") override;

  // === Members
  std::vector<double> values;
};

template <class T>
void SurfaceMesh::addVertexScalarQuantity(std::string name, const T& data, DataType type) {
  std::shared_ptr<SurfaceScalarQuantity> q = std::make_shared<SurfaceScalarVertexQuantity>(
      name, standardizeArray<double, T>(data, nVertices, "vertex scalar quantity " + name), this, type);
  addSurfaceQuantity(q);
}

// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

class SurfaceScalarFaceQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarFaceQuantity(std::string name, std::vector<double> values_, SurfaceMesh* mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildFaceInfoGUI(size_t fInd) override;

  // === Members
  std::vector<double> values;
};

template <class T>
void SurfaceMesh::addFaceScalarQuantity(std::string name, const T& data, DataType type) {
  std::shared_ptr<SurfaceScalarQuantity> q = std::make_shared<SurfaceScalarFaceQuantity>(
      name, standardizeArray<double, T>(data, nFaces, "face scalar quantity " + name), this, type);
  addSurfaceQuantity(q);
}

// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

class SurfaceScalarEdgeQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarEdgeQuantity(std::string name, std::vector<double> values_, SurfaceMesh* mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildEdgeInfoGUI(size_t edgeInd) override;


  // === Members
  std::vector<double> values;
};

template <class T>
void SurfaceMesh::addEdgeScalarQuantity(std::string name, const T& data, DataType type) {
  std::shared_ptr<SurfaceScalarQuantity> q = std::make_shared<SurfaceScalarEdgeQuantity>(
      name, standardizeArray<double, T>(data, nEdges, "edge scalar quantity " + name), this, type);
  addSurfaceQuantity(q);
}

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

class SurfaceScalarHalfedgeQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarHalfedgeQuantity(std::string name, std::vector<double> values_, SurfaceMesh* mesh_,
                                DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  void buildHalfedgeInfoGUI(size_t heInd) override;

  // === Members
  std::vector<double> values;
};

template <class T>
void SurfaceMesh::addHalfedgeScalarQuantity(std::string name, const T& data, DataType type) {
  std::shared_ptr<SurfaceScalarQuantity> q = std::make_shared<SurfaceScalarHalfedgeQuantity>(
      name, standardizeArray<double, T>(data, nHalfedges, "halfedge scalar quantity " + name), this, type);
  addSurfaceQuantity(q);
}

} // namespace polyscope
