// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

// Extend Quantity<> to add a few extra functions
class FloatingQuantity : public Quantity {
public:
  FloatingQuantity(std::string name, Structure& parentStructure);
  virtual ~FloatingQuantity(){};

  virtual void buildUI() override;

  virtual FloatingQuantity* setEnabled(bool newEnabled) = 0;
};


} // namespace polyscope
