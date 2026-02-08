// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/utilities.h"

namespace polyscope {

inline uint64_t SparseVolumeGrid::nCells() const { return occupiedCellsData.size(); }

inline glm::vec3 SparseVolumeGrid::getOrigin() const { return origin; }
inline glm::vec3 SparseVolumeGrid::getGridCellWidth() const { return gridCellWidth; }

// Template registration
template <class T>
SparseVolumeGrid* registerSparseVolumeGrid(std::string name, glm::vec3 origin, glm::vec3 gridCellWidth,
                                           const T& occupiedCells) {
  checkInitialized();

  SparseVolumeGrid* s = new SparseVolumeGrid(name, origin, gridCellWidth,
                                             standardizeVectorArray<glm::ivec3, 3>(occupiedCells));

  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}

// Shorthand to get a sparse volume grid from polyscope
inline SparseVolumeGrid* getSparseVolumeGrid(std::string name) {
  return dynamic_cast<SparseVolumeGrid*>(getStructure(SparseVolumeGrid::structureTypeName, name));
}
inline bool hasSparseVolumeGrid(std::string name) {
  return hasStructure(SparseVolumeGrid::structureTypeName, name);
}
inline void removeSparseVolumeGrid(std::string name, bool errorIfAbsent) {
  removeStructure(SparseVolumeGrid::structureTypeName, name, errorIfAbsent);
}

} // namespace polyscope
