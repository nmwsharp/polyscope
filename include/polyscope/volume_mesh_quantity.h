// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/structure.h"


namespace polyscope {

// Forward declare volume mesh
class VolumeMesh;

// Extend Quantity<VolumeMesh> to add a few extra functions
class VolumeMeshQuantity : public QuantityS<VolumeMesh> {
public:
  VolumeMeshQuantity(std::string name, VolumeMesh& parentStructure, bool dominates = false);
  ~VolumeMeshQuantity(){};
  virtual std::shared_ptr<render::ShaderProgram> createSliceProgram() { return nullptr; };
  virtual void drawSlice(polyscope::SlicePlane* sp){};

public:
  // Build GUI info about this element
  virtual void buildVertexInfoGUI(size_t vInd);
  virtual void buildEdgeInfoGUI(size_t eInd);
  virtual void buildFaceInfoGUI(size_t fInd);
  virtual void buildCellInfoGUI(size_t cInd);
};

} // namespace polyscope
