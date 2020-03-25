// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
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
  virtual void geometryChanged() override;

  // The map that takes values to [0,1] for drawing
  AffineRemapper<double> mapper;

  std::vector<std::pair<glm::vec3, double>> entries;

  const int NO_INDEX = std::numeric_limits<int>::min();
  int sum;

  // UI internals
  const std::string descriptiveType; // ("vertex count", etc)
  std::shared_ptr<render::ShaderProgram> program;

  // TODO use persistent/scaled quantities
  float pointRadius = 0.003;
  float vizRangeLow, vizRangeHigh, dataRangeLow, dataRangeHigh;
  std::string cMap = "coolwarm";

protected:
  void initializeLimits();
  void setUniforms(render::ShaderProgram& p);
  void createProgram();
};

// ========================================================
// ==========           Vertex Count             ==========
// ========================================================

class SurfaceVertexCountQuantity : public SurfaceCountQuantity {
public:
  SurfaceVertexCountQuantity(std::string name, const std::vector<std::pair<size_t, int>> values_, SurfaceMesh& mesh_);

  void buildVertexInfoGUI(size_t vInd) override;

  // === Members
  std::map<size_t, int> values;
};


// ========================================================
// ==========      Vertex Isolated Scalar        ==========
// ========================================================

class SurfaceVertexIsolatedScalarQuantity : public SurfaceCountQuantity {
public:
  SurfaceVertexIsolatedScalarQuantity(std::string name, const std::vector<std::pair<size_t, double>> values_,
                                      SurfaceMesh& mesh_);

  void buildVertexInfoGUI(size_t vInd) override;

  // === Members
  std::map<size_t, double> values;
};

// ========================================================
// ==========            Face Count             ==========
// ========================================================

class SurfaceFaceCountQuantity : public SurfaceCountQuantity {
public:
  SurfaceFaceCountQuantity(std::string name, const std::vector<std::pair<size_t, int>> values_, SurfaceMesh& mesh_);

  void buildFaceInfoGUI(size_t f) override;

  // === Members
  std::map<size_t, int> values;
};

} // namespace polyscope
