// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

class PointCloud;

class PointCloudQuantity : public Quantity {
public:
  PointCloudQuantity(std::string name, PointCloud& parentStructure, bool dominates = false);
  virtual ~PointCloudQuantity() {};

  PointCloud& parent; // shadows and hides the generic member in Quantity

  // Build GUI info about a point
  virtual void buildInfoGUI(size_t pointInd);
};


} // namespace polyscope
