// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/histogram.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/render/engine.h"

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
  
  // === Get/set visualization parameters

  // The color map
  SurfaceScalarQuantity* setColorMap(std::string val);
  std::string getColorMap();

  // Data limits mapped in to colormap
  SurfaceScalarQuantity* setMapRange(std::pair<double, double> val);
  std::pair<double, double> getMapRange();
  SurfaceScalarQuantity* resetMapRange(); // reset to full range

protected:
  // === Visualization parameters

  // Affine data maps and limits
  std::pair<float, float> vizRange;
  std::pair<double, double> dataRange;
  Histogram hist;

  // UI internals
  PersistentValue<std::string> cMap;
  const std::string definedOn;
  std::shared_ptr<render::ShaderProgram> program;

  // Helpers
  virtual void createProgram() = 0;
  void setProgramUniforms(render::ShaderProgram& program);
};

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

class SurfaceVertexScalarQuantity : public SurfaceScalarQuantity {
public:
  SurfaceVertexScalarQuantity(std::string name, std::vector<double> values_, SurfaceMesh& mesh_,
                              DataType dataType_ = DataType::STANDARD);

  virtual void createProgram() override;

  void fillColorBuffers(render::ShaderProgram& p);

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

  void fillColorBuffers(render::ShaderProgram& p);

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

  void fillColorBuffers(render::ShaderProgram& p);

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

  void fillColorBuffers(render::ShaderProgram& p);

  void buildHalfedgeInfoGUI(size_t heInd) override;

  // === Members
  std::vector<double> values;
};


} // namespace polyscope
