// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"


namespace polyscope {

// Forward declare structure
class VolumeGrid;

// Extend Quantity<VolumeGrid> to add a few extra functions
class VolumeGridQuantity : public QuantityS<VolumeGrid> {
public:
  VolumeGridQuantity(std::string name, VolumeGrid& parentStructure, bool dominates = false);
  ~VolumeGridQuantity(){};

  virtual bool isDrawingGridcubes() = 0;

  // Build GUI info about this element
  virtual void buildNodeInfoGUI(size_t vInd);
  virtual void buildCellInfoGUI(size_t vInd);
};

} // namespace polyscope
