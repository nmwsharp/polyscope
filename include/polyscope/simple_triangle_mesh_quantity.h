// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

class SimpleTriangleMesh;

class SimpleTriangleMeshQuantity : public Quantity {
public:
  SimpleTriangleMeshQuantity(std::string name, SimpleTriangleMesh& parentStructure, bool dominates = false);
  virtual ~SimpleTriangleMeshQuantity() {};

  SimpleTriangleMesh& parent; // shadows and hides the generic member in Quantity

  // Called by buildPickUI() to display this quantity's value for the selected element.
  // Override in vertex quantities (for vertex picks) and face quantities (for face picks).
  virtual void buildVertexInfoGUI(size_t vInd) {}
  virtual void buildFaceInfoGUI(size_t fInd) {}
};

} // namespace polyscope
