// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/render/engine.h"
#include "polyscope/scalar_quantity.h"
#include "polyscope/volume_mesh.h"

namespace polyscope {

class VolumeMeshScalarQuantity : public VolumeMeshQuantity, public ScalarQuantity<VolumeMeshScalarQuantity> {
public:
  VolumeMeshScalarQuantity(std::string name, VolumeMesh& mesh_, std::string definedOn, const std::vector<double>& values_,
                        DataType dataType);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;
  virtual void refresh() override;

protected:
  const std::string definedOn;
  std::shared_ptr<render::ShaderProgram> program;
  std::shared_ptr<render::ShaderProgram> sliceProgram;
  virtual std::shared_ptr<render::ShaderProgram> createSliceProgram(){ return nullptr; };

  // Helpers
  virtual void createProgram() = 0;
};

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

class VolumeMeshVertexScalarQuantity : public VolumeMeshScalarQuantity {
public:
  VolumeMeshVertexScalarQuantity(std::string name, const std::vector<double>& values_, VolumeMesh& mesh_,
                              DataType dataType_ = DataType::STANDARD);

  virtual void createProgram() override;
  virtual std::shared_ptr<render::ShaderProgram> createSliceProgram() override;
  virtual void draw() override;
  virtual void drawSlice(polyscope::SlicePlane *sp) override;

  void setLevelSetUniforms(render::ShaderProgram &p);
  void fillLevelSetData(render::ShaderProgram &p);
  std::shared_ptr<render::ShaderProgram> levelSetProgram;

  void fillColorBuffers(render::ShaderProgram& p);

  void fillGeomColorBuffers(render::ShaderProgram& p);

  virtual void buildCustomUI() override;
  void buildVertexInfoGUI(size_t vInd) override;
  virtual void refresh() override;

  float levelSetValue;
  bool isDrawingLevelSet;
  VolumeMeshVertexScalarQuantity* showQuantity;

};


// ========================================================
// ==========            Cell Scalar             ==========
// ========================================================

class VolumeMeshCellScalarQuantity : public VolumeMeshScalarQuantity {
public:
  VolumeMeshCellScalarQuantity(std::string name, const std::vector<double>& values_, VolumeMesh& mesh_,
                            DataType dataType_ = DataType::STANDARD);

  virtual void createProgram() override;

  void fillColorBuffers(render::ShaderProgram& p);

  void buildCellInfoGUI(size_t fInd) override;
};


} // namespace polyscope
