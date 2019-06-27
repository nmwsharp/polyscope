#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/colormap_sets.h"
#include "polyscope/histogram.h"
#include "polyscope/point_cloud.h"

#include <vector>

namespace polyscope {

class PointCloudScalarQuantity : public PointCloudQuantityThatDrawsPoints {
public:
  PointCloudScalarQuantity(std::string name, const std::vector<double>& values, PointCloud& pointCloud_,
                           DataType dataType);

  virtual void draw() override;
  virtual void buildCustomUI() override;

  void buildPickUI(size_t ind) override;

  virtual std::string niceName() override;

  // === Members
  std::vector<double> values;
  const DataType dataType;

  // Affine data maps and limits
  void resetVizRange();
  float vizRangeLow, vizRangeHigh;
  float dataRangeHigh, dataRangeLow;
  Histogram hist;

  // UI internals
  int iColorMap = 0;

protected:
  void createPointProgram();
};


template <class T>
void PointCloud::addScalarQuantity(std::string name, const T& data, DataType type) {

  validateSize(data, nPoints(), "point cloud scalar quantity " + name);

  PointCloudQuantityThatDrawsPoints* q =
      new PointCloudScalarQuantity(name, standardizeArray<double, T>(data), *this, type);
  addQuantity(q);
}


} // namespace polyscope
