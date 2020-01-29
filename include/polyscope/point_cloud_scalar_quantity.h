// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/color_maps.h"
#include "polyscope/histogram.h"
#include "polyscope/point_cloud.h"

#include <vector>

namespace polyscope {

class PointCloudScalarQuantity : public PointCloudQuantity {
public:
  PointCloudScalarQuantity(std::string name, const std::vector<double>& values, PointCloud& pointCloud_,
                           DataType dataType);

  virtual void draw() override;
  virtual void buildCustomUI() override;

  virtual void buildPickUI(size_t ind) override;
  virtual void geometryChanged() override;

  virtual std::string niceName() override;

  // === Members
  std::vector<double> values;
  const DataType dataType;

  // === Get/set visualization parameters

  // The color map
  PointCloudScalarQuantity* setColorMap(gl::ColorMapID val);
  gl::ColorMapID getColorMap();

  // Data limits mapped in to colormap
  PointCloudScalarQuantity* setMapRange(std::pair<double, double> val);
  std::pair<double, double> getMapRange();
  PointCloudScalarQuantity* resetMapRange(); // reset to full range

protected:
  // === Visualization parameters

  // Affine data maps and limits
  std::pair<float, float> vizRange;
  std::pair<double, double> dataRange;
  Histogram hist;

  // UI internals
  PersistentValue<gl::ColorMapID> cMap;


  void createPointProgram();
  std::unique_ptr<gl::GLProgram> pointProgram;
};


} // namespace polyscope
