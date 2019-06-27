#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/colormap_sets.h"
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
  int iColorMap = 0;
  const std::string definedOn;
  std::unique_ptr<gl::GLProgram> program;

  // Helpers
  virtual void createProgram() = 0;
  void setProgramUniforms(gl::GLProgram& program);
};

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

class SurfaceScalarVertexQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarVertexQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                              DataType dataType_ = DataType::STANDARD);

  virtual void createProgram() override;

  void fillColorBuffers(gl::GLProgram& p);

  void buildVertexInfoGUI(size_t vInd) override;
  virtual void writeToFile(std::string filename = "") override;

  // === Members
  std::vector<double> values;
};

template <class T>
void SurfaceMesh::addVertexScalarQuantity(std::string name, const T& data, DataType type) {

  validateSize(data, nVertices(), "vertex scalar quantity " + name);

  SurfaceScalarQuantity* q = new SurfaceScalarVertexQuantity(name, standardizeArray<double, T>(data), *this, type);
  addQuantity(q);
}

// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

class SurfaceScalarFaceQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarFaceQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                            DataType dataType_ = DataType::STANDARD);

  virtual void createProgram() override;

  void fillColorBuffers(gl::GLProgram& p);

  void buildFaceInfoGUI(size_t fInd) override;

  // === Members
  std::vector<double> values;
};

template <class T>
void SurfaceMesh::addFaceScalarQuantity(std::string name, const T& data, DataType type) {

  validateSize(data, nFaces(), "face scalar quantity " + name);

  SurfaceScalarQuantity* q = new SurfaceScalarFaceQuantity(name, standardizeArray<double, T>(data), *this, type);
  addQuantity(q);
}

// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

class SurfaceScalarEdgeQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarEdgeQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual void createProgram() override;

  void fillColorBuffers(gl::GLProgram& p);

  void buildEdgeInfoGUI(size_t edgeInd) override;


  // === Members
  std::vector<double> values;
};

template <class T>
void SurfaceMesh::addEdgeScalarQuantity(std::string name, const T& data, DataType type) {

  validateSize(data, nEdges(), "edge scalar quantity " + name);

  SurfaceScalarQuantity* q = new SurfaceScalarEdgeQuantity(name, standardizeArray<double, T>(data), *this, type);
  addQuantity(q);
}

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

class SurfaceScalarHalfedgeQuantity : public SurfaceScalarQuantity {
public:
  SurfaceScalarHalfedgeQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                                DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual void createProgram() override;

  void fillColorBuffers(gl::GLProgram& p);

  void buildHalfedgeInfoGUI(size_t heInd) override;

  // === Members
  std::vector<double> values;
};

template <class T>
void SurfaceMesh::addHalfedgeScalarQuantity(std::string name, const T& data, DataType type) {

  validateSize(data, {nHalfedges(), mesh.nRealHalfedges()}, "halfedge scalar quantity " + name);

  SurfaceScalarQuantity* q = new SurfaceScalarHalfedgeQuantity(name, standardizeArray<double, T>(data), *this, type);
  addQuantity(q);
}

} // namespace polyscope
