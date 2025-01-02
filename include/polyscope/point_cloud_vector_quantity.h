// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/point_cloud.h"
#include "polyscope/vector_quantity.h"

namespace polyscope {

// Represents a general vector field associated with a point cloud
class PointCloudVectorQuantity : public PointCloudQuantity, public VectorQuantity<PointCloudVectorQuantity> {

public:
  PointCloudVectorQuantity(std::string name, std::vector<glm::vec3> vectors, PointCloud& pointCloud_,
                           VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void buildPickUI(size_t ind) override;
  virtual std::string niceName() override;
  virtual void refresh() override;
};

} // namespace polyscope
