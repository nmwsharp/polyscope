// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/render/color_maps.h"
#include "polyscope/scalar_quantity.h"
#include "polyscope/sparse_volume_grid.h"
#include "polyscope/sparse_volume_grid_quantity.h"

#include <vector>

namespace polyscope {

class SparseVolumeGridScalarQuantity : public SparseVolumeGridQuantity,
                                       public ScalarQuantity<SparseVolumeGridScalarQuantity> {

public:
  // Cell scalar constructor
  SparseVolumeGridScalarQuantity(std::string name, SparseVolumeGrid& grid, const std::vector<float>& values,
                                 DataType dataType);

  // Node scalar constructor
  SparseVolumeGridScalarQuantity(std::string name, SparseVolumeGrid& grid, const std::vector<glm::ivec3>& nodeIndices,
                                 const std::vector<float>& nodeValues, DataType dataType);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;

  virtual std::string niceName() override;

  bool getNodeIndicesAreCanonical() const { return nodeIndicesAreCanonical; }

private:
  bool isNodeQuantity = false;
  bool nodeIndicesAreCanonical; // true if user-provided indices matched canonical order exactly (set by constructor)
  void createProgram();
  std::shared_ptr<render::ShaderProgram> program;
};


} // namespace polyscope
