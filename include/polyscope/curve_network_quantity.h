// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

// Forward declare
class CurveNetwork;

// Extend Quantity<CurveNetwork> 
class CurveNetworkQuantity : public Quantity<CurveNetwork> {
public:
  CurveNetworkQuantity(std::string name, CurveNetwork& parentStructure, bool dominates = false);
  virtual ~CurveNetworkQuantity() {};

  // Build GUI info an element
  virtual void buildNodeInfoGUI(size_t vInd);
  virtual void buildEdgeInfoGUI(size_t fInd);
};


} // namespace polyscope
