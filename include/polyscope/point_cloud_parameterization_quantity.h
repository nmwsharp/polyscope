// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/point_cloud.h"
#include "polyscope/point_cloud_quantity.h"

#include "polyscope/surface_parameterization_enums.h"

#include <vector>

namespace polyscope {

class PointCloudParameterizationQuantity : public PointCloudQuantity {
public:
  PointCloudParameterizationQuantity(std::string name, const std::vector<glm::vec2>& coords_, ParamCoordsType type_,
                                     ParamVizStyle style_, PointCloud& cloud_);

  virtual void draw() override;
  virtual void buildCustomUI() override;

  virtual void buildPickUI(size_t ind) override;
  virtual void refresh() override;

  virtual std::string niceName() override;

  // === Members
  std::vector<glm::vec2> coords;
  const ParamCoordsType coordsType;

  // === Getters and setters for visualization options

  // What visualization scheme to use
  PointCloudParameterizationQuantity* setStyle(ParamVizStyle newStyle);
  ParamVizStyle getStyle();

  // Colors for checkers
  PointCloudParameterizationQuantity* setCheckerColors(std::pair<glm::vec3, glm::vec3> colors);
  std::pair<glm::vec3, glm::vec3> getCheckerColors();

  // Colors for checkers
  PointCloudParameterizationQuantity* setGridColors(std::pair<glm::vec3, glm::vec3> colors);
  std::pair<glm::vec3, glm::vec3> getGridColors();

  // The size of checkers / stripes
  PointCloudParameterizationQuantity* setCheckerSize(double newVal);
  double getCheckerSize();

  // Color map for radial visualization
  PointCloudParameterizationQuantity* setColorMap(std::string val);
  std::string getColorMap();
  
  // Darkness for checkers (etc)
  PointCloudParameterizationQuantity* setAltDarkness(double newVal);
  double getAltDarkness();

protected:
  // === Visualiztion options
  PersistentValue<float> checkerSize;
  PersistentValue<ParamVizStyle> vizStyle;
  PersistentValue<glm::vec3> checkColor1, checkColor2;           // for checker (two colors to use)
  PersistentValue<glm::vec3> gridLineColor, gridBackgroundColor; // for GRID (two colors to use)
  PersistentValue<float> altDarkness;

  PersistentValue<std::string> cMap;
  float localRot = 0.; // for LOCAL (angular shift, in radians)

  void createProgram();
  void setProgramUniforms(render::ShaderProgram& program);
  std::shared_ptr<render::ShaderProgram> program;
};


} // namespace polyscope
