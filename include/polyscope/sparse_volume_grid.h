// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/color_management.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include <cstdint>
#include <vector>

namespace polyscope {

class SparseVolumeGrid;

class SparseVolumeGrid : public Structure {
public:
  // Construct a new sparse volume grid structure
  SparseVolumeGrid(std::string name, glm::vec3 origin, glm::vec3 gridCellWidth,
                   std::vector<glm::ivec3> occupiedCells);

  // === Overloads

  // Standard structure overrides
  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void drawPick() override;
  virtual void drawPickDelayed() override;
  virtual void updateObjectSpaceBounds() override;
  virtual std::string typeName() override;
  virtual void refresh() override;

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildCustomOptionsUI() override;
  virtual void buildPickUI(const PickResult& result) override;

  // Misc data
  static const std::string structureTypeName;

  // === Geometry members
  render::ManagedBuffer<glm::vec3> cellPositions;
  render::ManagedBuffer<glm::uvec3> cellIndices; // uvec3 for GPU; derived from signed occupiedCells

  // === Grid info
  uint64_t nCells() const;
  glm::vec3 getOrigin() const;
  glm::vec3 getGridCellWidth() const;

  // === Getters and setters for visualization settings

  // Color of the grid cubes
  SparseVolumeGrid* setColor(glm::vec3 val);
  glm::vec3 getColor();

  // Material
  SparseVolumeGrid* setMaterial(std::string name);
  std::string getMaterial();

  // Scaling factor for the size of the little cubes
  SparseVolumeGrid* setCubeSizeFactor(double newVal);
  double getCubeSizeFactor();

private:
  // Field data
  glm::vec3 origin;
  glm::vec3 gridCellWidth;

  // === Storage for managed quantities
  std::vector<glm::vec3> cellPositionsData;
  std::vector<glm::uvec3> cellIndicesData; // uvec3 for GPU attribute

  // User-facing occupied cell indices (signed)
  std::vector<glm::ivec3> occupiedCellsData;

  // === Visualization parameters
  PersistentValue<glm::vec3> color;
  PersistentValue<std::string> material;
  PersistentValue<float> cubeSizeFactor;

  // Compute cell positions and GPU indices from occupiedCellsData
  void computeCellPositions();

  // Picking-related
  size_t globalPickConstant = INVALID_IND_64;
  glm::vec3 pickColor;

  // Drawing related things
  std::shared_ptr<render::ShaderProgram> program;
  std::shared_ptr<render::ShaderProgram> pickProgram;

  // === Helpers
  void ensureRenderProgramPrepared();
  void ensurePickProgramPrepared();
};


// Register a sparse volume grid
template <class T>
SparseVolumeGrid* registerSparseVolumeGrid(std::string name, glm::vec3 origin, glm::vec3 gridCellWidth,
                                           const T& occupiedCells);

// Non-template overloads
SparseVolumeGrid* registerSparseVolumeGrid(std::string name, glm::vec3 origin, glm::vec3 gridCellWidth,
                                           const std::vector<glm::ivec3>& occupiedCells);

// Shorthand to get a sparse volume grid from polyscope
inline SparseVolumeGrid* getSparseVolumeGrid(std::string name = "");
inline bool hasSparseVolumeGrid(std::string name = "");
inline void removeSparseVolumeGrid(std::string name = "", bool errorIfAbsent = false);

} // namespace polyscope

#include "polyscope/sparse_volume_grid.ipp"
