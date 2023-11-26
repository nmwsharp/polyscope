// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/polyscope.h"

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/scalar_quantity.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/volume_grid.h"

namespace polyscope {

// ========================================================
// ==========            Node Scalar             ==========
// ========================================================

class VolumeGridNodeScalarQuantity : public VolumeGridQuantity, public ScalarQuantity<VolumeGridNodeScalarQuantity> {

public:
  VolumeGridNodeScalarQuantity(std::string name, VolumeGrid& grid_, const std::vector<float>& values_,
                               DataType dataType_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;
  virtual void buildNodeInfoGUI(size_t ind) override;

  virtual std::string niceName() override;

  virtual bool isDrawingGridcubes() override;

  // == Getters and setters

  // Gridcube viz

  VolumeGridNodeScalarQuantity* setGridcubeVizEnabled(bool val);
  bool getGridcubeVizEnabled();


  // Isosurface viz

  VolumeGridNodeScalarQuantity* setIsosurfaceVizEnabled(bool val);
  bool getIsosurfaceVizEnabled();

  VolumeGridNodeScalarQuantity* setIsosurfaceLevel(float value);
  float getIsosurfaceLevel();

  VolumeGridNodeScalarQuantity* setIsosurfaceColor(glm::vec3 val);
  glm::vec3 getIsosurfaceColor();

  VolumeGridNodeScalarQuantity* setSlicePlanesAffectIsosurface(bool val);
  bool getSlicePlanesAffectIsosurface();

  SurfaceMesh* registerIsosurfaceAsMesh(std::string structureName = "");

protected:
  // Visualize as a grid of cubes
  PersistentValue<bool> gridcubeVizEnabled;
  std::shared_ptr<render::ShaderProgram> gridcubeProgram;
  void createGridcubeProgram();

  // Visualize as isosurface
  // TODO
  PersistentValue<bool> isosurfaceVizEnabled;
  PersistentValue<float> isosurfaceLevel;
  PersistentValue<glm::vec3> isosurfaceColor;
  PersistentValue<bool> slicePlanesAffectIsosurface;
  std::shared_ptr<render::ShaderProgram> isosurfaceProgram;
  void createIsosurfaceProgram();

  // Visualize as raymarched volume
  // TODO
};


// ========================================================
// ==========            Cell Scalar             ==========
// ========================================================

class VolumeGridCellScalarQuantity : public VolumeGridQuantity, public ScalarQuantity<VolumeGridCellScalarQuantity> {

public:
  VolumeGridCellScalarQuantity(std::string name, VolumeGrid& grid_, const std::vector<float>& values_,
                               DataType dataType_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;
  virtual void buildCellInfoGUI(size_t ind) override;

  virtual std::string niceName() override;

  virtual bool isDrawingGridcubes() override;

  // == Getters and setters

  // Gridcube viz

  VolumeGridCellScalarQuantity* setGridcubeVizEnabled(bool val);
  bool getGridcubeVizEnabled();


protected:
  // Visualize as a grid of cubes
  PersistentValue<bool> gridcubeVizEnabled;
  std::shared_ptr<render::ShaderProgram> gridcubeProgram;
  void createGridcubeProgram();
};

} // namespace polyscope
