// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

namespace polyscope {

inline uint64_t SparseVolumeGrid::nCells() const { return occupiedCellsData.size(); }
inline uint64_t SparseVolumeGrid::nNodes() {
  ensureHaveCornerNodeIndices();
  return canonicalNodeIndsData.size();
}

inline glm::vec3 SparseVolumeGrid::getOrigin() const { return origin; }
inline glm::vec3 SparseVolumeGrid::getGridCellWidth() const { return gridCellWidth; }
inline const std::vector<glm::ivec3>& SparseVolumeGrid::getOccupiedCells() const { return occupiedCellsData; }
inline const std::vector<glm::ivec3>& SparseVolumeGrid::getCanonicalNodeInds() {
  ensureHaveCornerNodeIndices();
  return canonicalNodeIndsData;
}

// Template registration
template <class T>
SparseVolumeGrid* registerSparseVolumeGrid(std::string name, glm::vec3 origin, glm::vec3 gridCellWidth,
                                           const T& occupiedCells) {
  checkInitialized();

  SparseVolumeGrid* s =
      new SparseVolumeGrid(name, origin, gridCellWidth, standardizeVectorArray<glm::ivec3, 3>(occupiedCells));

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
inline bool hasSparseVolumeGrid(std::string name) { return hasStructure(SparseVolumeGrid::structureTypeName, name); }
inline void removeSparseVolumeGrid(std::string name, bool errorIfAbsent) {
  removeStructure(SparseVolumeGrid::structureTypeName, name, errorIfAbsent);
}

// =====================================================
// ============== Helpers
// =====================================================

template <typename T>
std::vector<T> SparseVolumeGrid::canonicalizeNodeValueArray(const std::string& quantityName,
                                                            const std::vector<glm::ivec3>& nodeIndices,
                                                            const std::vector<T>& nodeValues,
                                                            bool& nodeIndicesAreCanonical) {
  ensureHaveCornerNodeIndices();
  const std::vector<glm::ivec3>& canonical = canonicalNodeIndsData;
  nodeIndicesAreCanonical = true; // overwrite to false below if we find any issues

  // fast path: if it is already canonical, just copy the input array
  if (nodeIndices.size() == canonical.size()) {
    bool alreadyCanonical = true;
    for (size_t i = 0; i < canonical.size(); i++) {
      if (nodeIndices[i] != canonical[i]) {
        alreadyCanonical = false;
        break;
      }
    }
    if (alreadyCanonical) {
      nodeIndicesAreCanonical = true;
      return nodeValues; // already in canonical order, so just return it
    }
  }
  nodeIndicesAreCanonical = false;


  // Build sort permutation of user-provided indices into canonical (lexicographic) order
  auto ivec3Less = [](const glm::ivec3& a, const glm::ivec3& b) {
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    return a.z < b.z;
  };
  std::vector<size_t> order(nodeIndices.size());
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return ivec3Less(nodeIndices[a], nodeIndices[b]); });

  // Merge-walk sorted user indices against canonical indices
  std::vector<T> canonicalOutput(canonical.size());
  size_t ui = 0;
  for (size_t ci = 0; ci < canonical.size(); ci++) {

    // Any user entries less than canonical[ci] are extras
    while (ui < order.size() && ivec3Less(nodeIndices[order[ui]], canonical[ci])) {
      ui++;
    }

    // Must match canonical[ci]
    if (ui >= order.size() || nodeIndices[order[ui]] != canonical[ci]) {
      exception(quantityName + ": missing node value at node index (" + std::to_string(canonical[ci].x) + "," +
                std::to_string(canonical[ci].y) + "," + std::to_string(canonical[ci].z) + ")");
    }

    canonicalOutput[ci] = nodeValues[order[ui]];
    ui++;
  }

  return canonicalOutput;
}


// =====================================================
// ============== Quantities
// =====================================================

template <class T>
SparseVolumeGridCellScalarQuantity* SparseVolumeGrid::addCellScalarQuantity(std::string name, const T& values,
                                                                            DataType type) {
  validateSize(values, nCells(), "sparse volume grid cell scalar quantity " + name);
  return addCellScalarQuantityImpl(name, standardizeArray<float, T>(values), type);
}

template <class TI, class TV>
SparseVolumeGridNodeScalarQuantity* SparseVolumeGrid::addNodeScalarQuantity(std::string name, const TI& nodeIndices,
                                                                            const TV& nodeValues, DataType type) {
  if (adaptorF_size(nodeIndices) != adaptorF_size(nodeValues)) {
    exception("SparseVolumeGrid::addNodeScalarQuantity: nodeIndices and nodeValues must have the same size");
  }
  return addNodeScalarQuantityImpl(name, standardizeVectorArray<glm::ivec3, 3>(nodeIndices),
                                   standardizeArray<float, TV>(nodeValues), type);
}

template <class T>
SparseVolumeGridCellColorQuantity* SparseVolumeGrid::addCellColorQuantity(std::string name, const T& colors) {
  validateSize(colors, nCells(), "sparse volume grid cell color quantity " + name);
  return addCellColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}

template <class TI, class TC>
SparseVolumeGridNodeColorQuantity* SparseVolumeGrid::addNodeColorQuantity(std::string name, const TI& nodeIndices,
                                                                          const TC& nodeColors) {
  if (adaptorF_size(nodeIndices) != adaptorF_size(nodeColors)) {
    exception("SparseVolumeGrid::addNodeColorQuantity: nodeIndices and nodeColors must have the same size");
  }
  return addNodeColorQuantityImpl(name, standardizeVectorArray<glm::ivec3, 3>(nodeIndices),
                                  standardizeVectorArray<glm::vec3, 3>(nodeColors));
}


} // namespace polyscope
