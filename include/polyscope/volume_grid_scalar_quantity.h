// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/polyscope.h"

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/scalar_quantity.h"
#include "polyscope/volume_grid.h"

namespace polyscope {

class VolumeGridScalarQuantity : public VolumeGridQuantity, public ScalarQuantity<VolumeGridScalarQuantity> {

public:
  VolumeGridScalarQuantity(std::string name, VolumeGrid& grid_, const std::vector<double>& values_, DataType dataType_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;

  virtual std::string niceName() override;

  // == Getters and setters

  // Point viz

  VolumeGridScalarQuantity* setPointVizEnabled(bool val);
  bool getPointVizEnabled();


  // Isosurface viz

  VolumeGridScalarQuantity* setIsosurfaceVizEnabled(bool val);
  bool getIsosurfaceVizEnabled();

  VolumeGridScalarQuantity* setIsosurfaceLevel(float value);
  float getIsosurfaceLevel();

  VolumeGridScalarQuantity* setIsosurfaceColor(glm::vec3 val);
  glm::vec3 getIsosurfaceColor();


protected:
  void createProgram();
  void fillPositions();
  void resetMapRange();

  const DataType dataType;
  std::vector<double> values;
  std::vector<glm::vec3> positions;

  // Visualize as points
  PersistentValue<bool> pointVizEnabled;
  std::shared_ptr<render::ShaderProgram> pointProgram;
  void createPointProgram();

  // Visualize as isosurface
  // TODO
  PersistentValue<bool> isosurfaceVizEnabled;
  PersistentValue<float> isosurfaceLevel;
  PersistentValue<glm::vec3> isosurfaceColor;
  std::shared_ptr<render::ShaderProgram> isosurfaceProgram;
  void createIsosurfaceProgram();

  // Visualize as raymarched volume
};

} // namespace polyscope
