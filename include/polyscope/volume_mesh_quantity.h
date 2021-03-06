// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"


namespace polyscope {

// Forward declare volume mesh
class VolumeMesh;

// Extend Quantity<VolumeMesh> to add a few extra functions
class VolumeMeshQuantity : public Quantity<VolumeMesh> {
public:
  VolumeMeshQuantity(std::string name, VolumeMesh& parentStructure, bool dominates = false);
  ~VolumeMeshQuantity() {};

public:
  // Build GUI info about this element
  virtual void buildVertexInfoGUI(size_t vInd);
  virtual void buildEdgeInfoGUI(size_t eInd);
  virtual void buildFaceInfoGUI(size_t fInd);
  virtual void buildCellInfoGUI(size_t cInd);
};

} // namespace polyscope
