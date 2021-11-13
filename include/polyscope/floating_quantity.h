// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

// Forward delcare structure (& global getter)
class FloatingQuantityStructure;
FloatingQuantityStructure* getGlobalFloatingQuantityStructure();

// Extend Quantity<> to add a few extra functions
class FloatingQuantity : public Quantity<FloatingQuantityStructure> {
public:
  FloatingQuantity(std::string name, FloatingQuantityStructure& parentStructure, bool dominates = false);
  virtual ~FloatingQuantity() {};
};


} // namespace polyscope
