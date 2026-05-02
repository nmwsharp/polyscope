// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

class CurveNetwork;

class CurveNetworkQuantity : public Quantity {
public:
  CurveNetworkQuantity(std::string name, CurveNetwork& parentStructure, bool dominates = false);
  virtual ~CurveNetworkQuantity() {};

  CurveNetwork& parent; // shadows and hides the generic member in Quantity

  // Build GUI info an element
  virtual void buildNodeInfoGUI(size_t vInd);
  virtual void buildEdgeInfoGUI(size_t fInd);
};


} // namespace polyscope
