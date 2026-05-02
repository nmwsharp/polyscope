// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/quantity.h"
#include "polyscope/slice_plane.h"
#include "polyscope/structure.h"


namespace polyscope {

class VolumeMesh;

class VolumeMeshQuantity : public Quantity {
public:
  VolumeMeshQuantity(std::string name, VolumeMesh& parentStructure, bool dominates = false);
  ~VolumeMeshQuantity() {};

  VolumeMesh& parent; // shadows and hides the generic member in Quantity

  virtual std::shared_ptr<render::ShaderProgram> createSliceProgram() { return nullptr; };
  virtual void drawSlice(polyscope::SlicePlane* sp) {};

public:
  // Build GUI info about this element
  virtual void buildVertexInfoGUI(size_t vInd);
  virtual void buildEdgeInfoGUI(size_t eInd);
  virtual void buildFaceInfoGUI(size_t fInd);
  virtual void buildCellInfoGUI(size_t cInd);
};

} // namespace polyscope
