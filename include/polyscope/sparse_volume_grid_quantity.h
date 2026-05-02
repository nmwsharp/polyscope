// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

class SparseVolumeGrid;

class SparseVolumeGridQuantity : public Quantity {
public:
  SparseVolumeGridQuantity(std::string name, SparseVolumeGrid& parentStructure, bool dominates = false);
  virtual ~SparseVolumeGridQuantity() {};

  // Build info GUI for picked elements (overridden by subclasses to display quantity values)
  virtual void buildCellInfoGUI(size_t cellInd);
  virtual void buildNodeInfoGUI(size_t nodeInd);

  SparseVolumeGrid& parent; // shadows and hides the generic member in Quantity
};


} // namespace polyscope
