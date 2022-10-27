// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/point_cloud.h"
#include "polyscope/point_cloud_quantity.h"

#include "polyscope/surface_parameterization_enums.h"

#include <vector>

namespace polyscope {

// TODO make a ParameterizationQuantity.h

class PointCloudParameterizationQuantity : public PointCloudQuantity {
public:
  PointCloudParameterizationQuantity(std::string name, const std::vector<glm::vec2>& coords_, ParamCoordsType type_,
                                     ParamVizStyle style_, PointCloud& cloud_);

  virtual void draw() override;
  virtual void buildCustomUI() override;

  virtual void buildPickUI(size_t ind) override;
  virtual void refresh() override;

  virtual std::string niceName() override;

  template <class V>
  void updateData(const V& newCoords);

  void ensureRenderBuffersFilled(bool forceRefill = false);

  std::shared_ptr<render::AttributeBuffer> getCoordRenderBuffer();

  // === Members
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

  // === ~DANGER~ experimental/unsupported functions

  uint32_t getCoordBufferID();
  void bufferDataExternallyUpdated();


protected:
  std::vector<glm::vec2> coords;

  // === Visualiztion options
  PersistentValue<float> checkerSize;
  PersistentValue<ParamVizStyle> vizStyle;
  PersistentValue<glm::vec3> checkColor1, checkColor2;           // for checker (two colors to use)
  PersistentValue<glm::vec3> gridLineColor, gridBackgroundColor; // for GRID (two colors to use)
  PersistentValue<float> altDarkness;

  PersistentValue<std::string> cMap;
  float localRot = 0.; // for LOCAL (angular shift, in radians)

  void createProgram();
  void dataUpdated();
  void setProgramUniforms(render::ShaderProgram& program);

  std::shared_ptr<render::AttributeBuffer> coordRenderBuffer;
  std::shared_ptr<render::ShaderProgram> pointProgram;
};


} // namespace polyscope

#include "polyscope/point_cloud_parameterization_quantity.ipp"
