#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/surface_mesh.h"

#include <map>

namespace polyscope {

class SurfaceCountQuantity : public SurfaceQuantity {
public:
  SurfaceCountQuantity(std::string name, SurfaceMesh* mesh_, std::string descriptiveType_);

  virtual void draw() override;
  virtual void drawUI() override;
  void prepare();

  // The map that takes values to [0,1] for drawing
  AffineRemapper<double> mapper;

  std::vector<std::pair<Vector3, double>> entries;

  const int NO_INDEX = std::numeric_limits<int>::min();
  gl::GLProgram* program = nullptr;
  int sum;

  // UI internals
  const std::string descriptiveType; // ("vertex count", etc)

protected:
  void setUniforms(gl::GLProgram* p);
  float pointRadius = 0.012;
  float vizRangeLow, vizRangeHigh, dataRangeLow, dataRangeHigh;
  int iColorMap;
};

// ========================================================
// ==========           Vertex Count             ==========
// ========================================================

class SurfaceCountVertexQuantity : public SurfaceCountQuantity {
public:
  SurfaceCountVertexQuantity(std::string name, std::vector<std::pair<VertexPtr, int>>& values_, SurfaceMesh* mesh_);
  //   ~SurfaceCountVertexQuantity();

  void buildInfoGUI(VertexPtr v) override;

  // === Members
  std::map<VertexPtr, int> values;
};

// ========================================================
// ==========      Vertex Isolated Scalar        ==========
// ========================================================

class SurfaceIsolatedScalarVertexQuantity : public SurfaceCountQuantity {
public:
  SurfaceIsolatedScalarVertexQuantity(std::string name, std::vector<std::pair<VertexPtr, double>>& values_,
                                      SurfaceMesh* mesh_);
  //   ~SurfaceCountVertexQuantity();

  void drawUI() override;
  void buildInfoGUI(VertexPtr v) override;

  // === Members
  std::map<VertexPtr, double> values;
};

// ========================================================
// ==========            Face Count             ==========
// ========================================================

class SurfaceCountFaceQuantity : public SurfaceCountQuantity {
public:
  SurfaceCountFaceQuantity(std::string name, std::vector<std::pair<FacePtr, int>>& values_, SurfaceMesh* mesh_);
  //   ~SurfaceCountVertexQuantity();

  void buildInfoGUI(FacePtr f) override;

  // === Members
  std::map<FacePtr, int> values;
};


} // namespace polyscope
