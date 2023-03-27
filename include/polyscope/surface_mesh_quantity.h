// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"


namespace polyscope {

// Forward delcare surface mesh
class SurfaceMesh;

// Extend Quantity<SurfaceMesh> to add a few extra functions
class SurfaceMeshQuantity : public QuantityS<SurfaceMesh> {
public:
  SurfaceMeshQuantity(std::string name, SurfaceMesh& parentStructure, bool dominates = false);
  ~SurfaceMeshQuantity(){};

public:
  // Build GUI info about this element
  virtual void buildVertexInfoGUI(size_t vInd);
  virtual void buildFaceInfoGUI(size_t fInd);
  virtual void buildEdgeInfoGUI(size_t eInd);
  virtual void buildHalfedgeInfoGUI(size_t heInd);
  virtual void buildCornerInfoGUI(size_t heInd);
};

} // namespace polyscope
