// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/utilities.h"

namespace polyscope {

inline uint64_t SparseVolumeGrid::nCells() const { return occupiedCellsData.size(); }

inline glm::vec3 SparseVolumeGrid::getOrigin() const { return origin; }
inline glm::vec3 SparseVolumeGrid::getGridCellWidth() const { return gridCellWidth; }
inline const std::vector<glm::ivec3>& SparseVolumeGrid::getOccupiedCells() const { return occupiedCellsData; }

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

// =====================================================
// ============== Quantities
// =====================================================

template <class T>
SparseVolumeGridScalarQuantity* SparseVolumeGrid::addCellScalarQuantity(std::string name, const T& values,
                                                                        DataType type) {
  validateSize(values, nCells(), "sparse volume grid cell scalar quantity " + name);
  return addCellScalarQuantityImpl(name, standardizeArray<float, T>(values), type);
}

template <class TI, class TV>
SparseVolumeGridScalarQuantity* SparseVolumeGrid::addNodeScalarQuantity(std::string name, const TI& nodeIndices,
                                                                        const TV& nodeValues, DataType type) {
  if (adaptorF_size(nodeIndices) != adaptorF_size(nodeValues)) {
    exception("SparseVolumeGrid::addNodeScalarQuantity: nodeIndices and nodeValues must have the same size");
  }
  return addNodeScalarQuantityImpl(name, standardizeVectorArray<glm::ivec3, 3>(nodeIndices),
                                   standardizeArray<float, TV>(nodeValues), type);
}

template <class T>
SparseVolumeGridColorQuantity* SparseVolumeGrid::addCellColorQuantity(std::string name, const T& colors) {
  validateSize(colors, nCells(), "sparse volume grid cell color quantity " + name);
  return addCellColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}

template <class TI, class TC>
SparseVolumeGridColorQuantity* SparseVolumeGrid::addNodeColorQuantity(std::string name, const TI& nodeIndices,
                                                                      const TC& nodeColors) {
  if (adaptorF_size(nodeIndices) != adaptorF_size(nodeColors)) {
    exception("SparseVolumeGrid::addNodeColorQuantity: nodeIndices and nodeColors must have the same size");
  }
  return addNodeColorQuantityImpl(name, standardizeVectorArray<glm::ivec3, 3>(nodeIndices),
                                  standardizeVectorArray<glm::vec3, 3>(nodeColors));
}


} // namespace polyscope
