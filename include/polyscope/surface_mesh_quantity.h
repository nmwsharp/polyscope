// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"


namespace polyscope {

class SurfaceMesh;

class SurfaceMeshQuantity : public Quantity {
public:
  SurfaceMeshQuantity(std::string name, SurfaceMesh& parentStructure, bool dominates = false);
  ~SurfaceMeshQuantity() {};

  SurfaceMesh& parent; // shadows and hides the generic member in Quantity

public:
  // Build GUI info about this element
  virtual void buildVertexInfoGUI(size_t vInd);
  virtual void buildFaceInfoGUI(size_t fInd);
  virtual void buildEdgeInfoGUI(size_t eInd);
  virtual void buildHalfedgeInfoGUI(size_t heInd);
  virtual void buildCornerInfoGUI(size_t heInd);
};

} // namespace polyscope
