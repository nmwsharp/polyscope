// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

// Forward declare
class VolumetricGrid;

// Extend Quantity<CurveNetwork> 
class VolumetricGridQuantity : public Quantity<VolumetricGrid> {
public:
  VolumetricGridQuantity(std::string name, VolumetricGrid& parentStructure, bool dominates = false);
  virtual ~VolumetricGridQuantity() {};

  // Build GUI info an element
  virtual void buildNodeInfoGUI(size_t vInd);
  virtual void buildEdgeInfoGUI(size_t fInd);

  // Invalidate geometric data
  virtual void geometryChanged() = 0;
};


} // namespace polyscope
