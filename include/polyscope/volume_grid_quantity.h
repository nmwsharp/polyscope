// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"


namespace polyscope {

class VolumeGrid;

class VolumeGridQuantity : public Quantity {
public:
  VolumeGridQuantity(std::string name, VolumeGrid& parentStructure, bool dominates = false);
  ~VolumeGridQuantity() {};

  VolumeGrid& parent; // shadows and hides the generic member in Quantity

  virtual bool isDrawingGridcubes() = 0;

  // Build GUI info about this element
  virtual void buildNodeInfoGUI(size_t vInd);
  virtual void buildCellInfoGUI(size_t vInd);
};

} // namespace polyscope
