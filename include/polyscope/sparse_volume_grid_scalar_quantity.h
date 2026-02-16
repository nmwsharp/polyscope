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
  SparseVolumeGridScalarQuantity(std::string name, SparseVolumeGrid& grid, const std::string& definedOn_,
                                 const std::vector<float>& values_, DataType dataType_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;

  virtual std::string niceName() override;

protected:
  const std::string definedOn;
  std::shared_ptr<render::ShaderProgram> program;
  virtual void createProgram() = 0;
};

// ========================================================
// ==========            Cell Scalar             ==========
// ========================================================

class SparseVolumeGridCellScalarQuantity : public SparseVolumeGridScalarQuantity {
public:
  SparseVolumeGridCellScalarQuantity(std::string name, SparseVolumeGrid& grid, const std::vector<float>& cellValues,
                                     DataType dataType);

  virtual void createProgram() override;
  virtual void buildCellInfoGUI(size_t cellInd) override;
};


// ========================================================
// ==========            Node Scalar             ==========
// ========================================================

class SparseVolumeGridNodeScalarQuantity : public SparseVolumeGridScalarQuantity {
public:
  SparseVolumeGridNodeScalarQuantity(std::string name, SparseVolumeGrid& grid,
                                     const std::vector<glm::ivec3>& nodeIndices, const std::vector<float>& nodeValues,
                                     DataType dataType);

  virtual void createProgram() override;
  virtual void buildNodeInfoGUI(size_t nodeInd) override;
  bool getNodeIndicesAreCanonical() const { return nodeIndicesAreCanonical; }

protected:
  bool nodeIndicesAreCanonical; // true if user-provided indices matched canonical order exactly (set by constructor)
};

} // namespace polyscope
