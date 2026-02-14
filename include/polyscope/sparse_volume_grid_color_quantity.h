// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/color_quantity.h"
#include "polyscope/sparse_volume_grid.h"
#include "polyscope/sparse_volume_grid_quantity.h"

#include <vector>

namespace polyscope {

class SparseVolumeGridColorQuantity : public SparseVolumeGridQuantity,
                                      public ColorQuantity<SparseVolumeGridColorQuantity> {
public:
  // Cell color constructor
  SparseVolumeGridColorQuantity(std::string name, SparseVolumeGrid& grid, const std::vector<glm::vec3>& colors);

  // Node color constructor
  SparseVolumeGridColorQuantity(std::string name, SparseVolumeGrid& grid, const std::vector<glm::ivec3>& nodeIndices,
                                const std::vector<glm::vec3>& nodeColors);

  virtual void draw() override;
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
