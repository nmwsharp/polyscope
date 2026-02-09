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

  SparseVolumeGrid& parent; // shadows and hides the generic member in Quantity
};


} // namespace polyscope
