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

  std::vector<std::pair<glm::vec3, double>> entries;

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
  SurfaceCountVertexQuantity(std::string name, std::vector<std::pair<size_t, int>>& values_, SurfaceMesh* mesh_);
  //   ~SurfaceCountVertexQuantity();

  void buildVertexInfoGUI(size_t vInd) override;

  // === Members
  std::map<size_t, int> values;
};

// ========================================================
// ==========      Vertex Isolated Scalar        ==========
// ========================================================

class SurfaceIsolatedScalarVertexQuantity : public SurfaceCountQuantity {
public:
  SurfaceIsolatedScalarVertexQuantity(std::string name, std::vector<std::pair<size_t, double>>& values_,
                                      SurfaceMesh* mesh_);
  //   ~SurfaceCountVertexQuantity();

  void drawUI() override;
  void buildVertexInfoGUI(size_t vInd) override;

  // === Members
  std::map<size_t, double> values;
};

// ========================================================
// ==========            Face Count             ==========
// ========================================================

class SurfaceCountFaceQuantity : public SurfaceCountQuantity {
public:
  SurfaceCountFaceQuantity(std::string name, std::vector<std::pair<size_t, int>>& values_, SurfaceMesh* mesh_);
  //   ~SurfaceCountVertexQuantity();

  void buildFaceInfoGUI(size_t f) override;

  // === Members
  std::map<size_t, int> values;
};


} // namespace polyscope
