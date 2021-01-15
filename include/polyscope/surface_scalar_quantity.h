// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/render/engine.h"
#include "polyscope/scalar_quantity.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceScalarQuantity : public SurfaceMeshQuantity, public ScalarQuantity<SurfaceScalarQuantity> {
public:
  SurfaceScalarQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn, const std::vector<double>& values_,
                        DataType dataType);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;
  virtual void refresh() override;

protected:
  const std::string definedOn;
  std::shared_ptr<render::ShaderProgram> program;

  // Helpers
  virtual void createProgram() = 0;
};

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

class SurfaceVertexScalarQuantity : public SurfaceScalarQuantity {
public:
  SurfaceVertexScalarQuantity(std::string name, const std::vector<double>& values_, SurfaceMesh& mesh_,
                              DataType dataType_ = DataType::STANDARD);

  virtual void createProgram() override;

  void fillColorBuffers(render::ShaderProgram& p);

  void buildVertexInfoGUI(size_t vInd) override;
};


// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

class SurfaceFaceScalarQuantity : public SurfaceScalarQuantity {
public:
  SurfaceFaceScalarQuantity(std::string name, const std::vector<double>& values_, SurfaceMesh& mesh_,
                            DataType dataType_ = DataType::STANDARD);

  virtual void createProgram() override;

  void fillColorBuffers(render::ShaderProgram& p);

  void buildFaceInfoGUI(size_t fInd) override;
};


// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

class SurfaceEdgeScalarQuantity : public SurfaceScalarQuantity {
public:
  SurfaceEdgeScalarQuantity(std::string name, const std::vector<double>& values_, SurfaceMesh& mesh_,
                            DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceVertexScalarQuantity();

  virtual void createProgram() override;

  void fillColorBuffers(render::ShaderProgram& p);

  void buildEdgeInfoGUI(size_t edgeInd) override;
};

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

class SurfaceHalfedgeScalarQuantity : public SurfaceScalarQuantity {
public:
  SurfaceHalfedgeScalarQuantity(std::string name, const std::vector<double>& values_, SurfaceMesh& mesh_,
                                DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceVertexScalarQuantity();

  virtual void createProgram() override;

  void fillColorBuffers(render::ShaderProgram& p);

  void buildHalfedgeInfoGUI(size_t heInd) override;
};


} // namespace polyscope
