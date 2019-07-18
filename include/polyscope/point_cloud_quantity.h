// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

// Forward delcare surface mesh
class PointCloud;

// Extend Quantity<SurfaceMesh> to add a few extra functions
class PointCloudQuantity : public Quantity<PointCloud> {
public:
  PointCloudQuantity(std::string name, PointCloud& parentStructure, bool dominates = false);

  // Build GUI info about a point
  virtual void buildInfoGUI(size_t pointInd);
};


} // namespace polyscope
