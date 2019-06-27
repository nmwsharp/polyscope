#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/surface_mesh.h"

#include <map>

namespace polyscope {

class SurfaceCountQuantity : public SurfaceMeshQuantity {
public:
  SurfaceCountQuantity(std::string name, SurfaceMesh& mesh_, std::string descriptiveType_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;

  // The map that takes values to [0,1] for drawing
  AffineRemapper<double> mapper;

  std::vector<std::pair<glm::vec3, double>> entries;

  const int NO_INDEX = std::numeric_limits<int>::min();
  int sum;

  // UI internals
  const std::string descriptiveType; // ("vertex count", etc)
  std::unique_ptr<gl::GLProgram> program;

protected:
  void setUniforms(gl::GLProgram& p);
  float pointRadius = 0.003;
  float vizRangeLow, vizRangeHigh, dataRangeLow, dataRangeHigh;
  int iColorMap;

  void createProgram();
};

// ========================================================
// ==========           Vertex Count             ==========
// ========================================================

class SurfaceCountVertexQuantity : public SurfaceCountQuantity {
public:
  SurfaceCountVertexQuantity(std::string name, const std::vector<std::pair<size_t, int>> values_, SurfaceMesh& mesh_);

  void buildVertexInfoGUI(size_t vInd) override;

  // === Members
  std::map<size_t, int> values;
};


inline void SurfaceMesh::addVertexCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>& values) {
  SurfaceCountQuantity* q = new SurfaceCountVertexQuantity(name, values, *this);
  addQuantity(q);
}

// ========================================================
// ==========      Vertex Isolated Scalar        ==========
// ========================================================

class SurfaceIsolatedScalarVertexQuantity : public SurfaceCountQuantity {
public:
  SurfaceIsolatedScalarVertexQuantity(std::string name, const std::vector<std::pair<size_t, double>> values_,
                                      SurfaceMesh& mesh_);

  void buildVertexInfoGUI(size_t vInd) override;

  // === Members
  std::map<size_t, double> values;
};

inline void SurfaceMesh::addIsolatedVertexScalarQuantity(std::string name,
                                                         const std::vector<std::pair<size_t, double>>& values) {
  SurfaceIsolatedScalarVertexQuantity* q = new SurfaceIsolatedScalarVertexQuantity(name, values, *this);
  addQuantity(q);
}

// ========================================================
// ==========            Face Count             ==========
// ========================================================

class SurfaceCountFaceQuantity : public SurfaceCountQuantity {
public:
  SurfaceCountFaceQuantity(std::string name, const std::vector<std::pair<size_t, int>> values_, SurfaceMesh& mesh_);

  void buildFaceInfoGUI(size_t f) override;

  // === Members
  std::map<size_t, int> values;
};

inline void SurfaceMesh::addFaceCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>& values) {
  SurfaceCountQuantity* q = new SurfaceCountFaceQuantity(name, values, *this);
  addQuantity(q);
}

} // namespace polyscope
