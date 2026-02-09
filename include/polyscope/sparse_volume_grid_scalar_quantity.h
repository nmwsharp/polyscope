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

private:
  bool isNodeQuantity = false;
  void createProgram();
  std::shared_ptr<render::ShaderProgram> program;

  // Node-mode packed data (8 corner values per cell, packed into 2 vec4)
  std::vector<glm::vec4> nodeValues04Data, nodeValues47Data;
  std::unique_ptr<render::ManagedBuffer<glm::vec4>> nodeValues04, nodeValues47;

  void packNodeValues(const std::vector<glm::ivec3>& nodeIndices, const std::vector<float>& nodeValues);
};


} // namespace polyscope
