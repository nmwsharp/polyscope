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
  // Node color constructor
  SparseVolumeGridColorQuantity(std::string name, SparseVolumeGrid& grid, const std::string& definedOn_,
                                const std::vector<glm::vec3>& colors_);

  virtual void draw() override;
  virtual void refresh() override;

  virtual std::string niceName() override;

protected:
  const std::string definedOn;
  std::shared_ptr<render::ShaderProgram> program;
  virtual void createProgram() = 0;
};

// ========================================================
// ==========            Cell Color              ==========
// ========================================================

class SparseVolumeGridCellColorQuantity : public SparseVolumeGridColorQuantity {
public:
  SparseVolumeGridCellColorQuantity(std::string name, SparseVolumeGrid& grid, const std::vector<glm::vec3>& cellColors);

  virtual void createProgram() override;
  virtual void buildCellInfoGUI(size_t cellInd) override;
};


// ========================================================
// ==========            Node Color              ==========
// ========================================================

class SparseVolumeGridNodeColorQuantity : public SparseVolumeGridColorQuantity {
public:
  SparseVolumeGridNodeColorQuantity(std::string name, SparseVolumeGrid& grid,
                                    const std::vector<glm::ivec3>& nodeIndices,
                                    const std::vector<glm::vec3>& nodeColors);

  virtual void createProgram() override;
  virtual void buildNodeInfoGUI(size_t nodeInd) override;
  bool getNodeIndicesAreCanonical() const { return nodeIndicesAreCanonical; }

protected:
  bool nodeIndicesAreCanonical; // true if user-provided indices matched canonical order exactly (set by constructor)
};

} // namespace polyscope
