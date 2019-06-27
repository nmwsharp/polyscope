#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/colormap_sets.h"
#include "polyscope/histogram.h"
#include "polyscope/point_cloud.h"

#include <vector>

namespace polyscope {

class PointCloudColorQuantity : public PointCloudQuantityThatDrawsPoints {
public:
  PointCloudColorQuantity(std::string name, const std::vector<glm::vec3>& values, PointCloud& pointCloud_);

  virtual void draw() override;

  void buildPickUI(size_t ind) override;

  virtual std::string niceName() override;

  // === Members
  std::vector<glm::vec3> values;

protected:
  void createPointProgram();
};


template <class T>
void PointCloud::addColorQuantity(std::string name, const T& colors) {

  validateSize(colors, nPoints(), "point cloud color quantity " + name);

  PointCloudQuantityThatDrawsPoints* q =
      new PointCloudColorQuantity(name, standardizeVectorArray<glm::vec3, T, 3>(colors), *this);
  this->addQuantity(q);
}

} // namespace polyscope
