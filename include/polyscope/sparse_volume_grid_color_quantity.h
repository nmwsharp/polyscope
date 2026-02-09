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

private:
  bool isNodeQuantity = false;
  void createProgram();
  std::shared_ptr<render::ShaderProgram> program;

  // Node-mode packed data (8 corner colors, separated by R/G/B channel, 2 vec4 each)
  std::vector<glm::vec4> nodeR04Data, nodeR47Data, nodeG04Data, nodeG47Data, nodeB04Data, nodeB47Data;
  std::unique_ptr<render::ManagedBuffer<glm::vec4>> nodeR04, nodeR47, nodeG04, nodeG47, nodeB04, nodeB47;

  void packNodeColors(const std::vector<glm::ivec3>& nodeIndices, const std::vector<glm::vec3>& nodeColors);
};


} // namespace polyscope
