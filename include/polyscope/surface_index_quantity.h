#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceIndexQuantity : public SurfaceQuantity {
public:
  SurfaceIndexQuantity(std::string name, SurfaceMesh* mesh_, std::string definedOn);

  virtual void draw() override;
  virtual void drawUI() override;
  void prepare();

  // The map that takes values to [0,1] for drawing
  AffineRemapper<double> mapper;

  std::vector<std::pair<Vector3, int>> entries;

  const int NO_INDEX = std::numeric_limits<int>::min();
  gl::GLProgram* program = nullptr;
  int sum;

  // UI internals
  const std::string definedOn;

private:
  void setPointCloudBillboardUniforms(gl::GLProgram* p, bool withLight);
  float pointRadius = 0.012;
};

// ========================================================
// ==========           Vertex Index             ==========
// ========================================================

class SurfaceIndexVertexQuantity : public SurfaceIndexQuantity {
public:
  SurfaceIndexVertexQuantity(std::string name, std::vector<std::pair<VertexPtr, int>>& values_, SurfaceMesh* mesh_);
  //   ~SurfaceIndexVertexQuantity();

  void buildInfoGUI(VertexPtr v) override;

  // === Members
  VertexData<int> values;
};

// ========================================================
// ==========            Face Index             ==========
// ========================================================

class SurfaceIndexFaceQuantity : public SurfaceIndexQuantity {
public:
  SurfaceIndexFaceQuantity(std::string name, std::vector<std::pair<FacePtr, int>>& values_, SurfaceMesh* mesh_);
  //   ~SurfaceIndexVertexQuantity();

  void buildInfoGUI(FacePtr f) override;

  // === Members
  FaceData<int> values;
};


} // namespace polyscope