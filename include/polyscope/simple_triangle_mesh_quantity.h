// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

class SimpleTriangleMesh;

class SimpleTriangleMeshQuantity : public Quantity {
public:
  SimpleTriangleMeshQuantity(std::string name, SimpleTriangleMesh& parentStructure, bool dominates = false);
  virtual ~SimpleTriangleMeshQuantity(){};

  SimpleTriangleMesh& parent; // shadows and hides the generic member in Quantity
};

} // namespace polyscope
