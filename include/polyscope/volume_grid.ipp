// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

namespace polyscope {

inline uint64_t VolumeGrid::nNodes() const {
  return static_cast<uint64_t>(gridNodeDim.x) * gridNodeDim.y * gridNodeDim.z;
}

inline uint64_t VolumeGrid::nCells() const {
  return static_cast<uint64_t>(gridCellDim.x) * gridCellDim.y * gridCellDim.z;
}


// Field data
inline glm::uvec3 VolumeGrid::getGridNodeDim() const { return gridNodeDim; }
inline glm::uvec3 VolumeGrid::getGridCellDim() const { return gridCellDim; }
inline glm::vec3 VolumeGrid::getBoundMin() const { return boundMin; }
inline glm::vec3 VolumeGrid::getBoundMax() const { return boundMax; }

inline uint64_t VolumeGrid::flattenNodeIndex(glm::uvec3 inds) const {
  return static_cast<uint64_t>(gridNodeDim[0]) * gridNodeDim[1] * inds.z + gridNodeDim[0] * inds.y + inds.x;
}

inline glm::uvec3 VolumeGrid::unflattenNodeIndex(uint64_t i) const {
  uint64_t nXY = gridNodeDim[0] * gridNodeDim[1];
  uint64_t iZ = i / nXY;
  i -= iZ * nXY;
  uint64_t nX = gridNodeDim[0];
  uint64_t iY = i / nX;
  i -= iY * nX;
  uint64_t iX = i;
  return glm::uvec3{static_cast<uint32_t>(iX), static_cast<uint32_t>(iY), static_cast<uint32_t>(iZ)};
}

inline glm::vec3 VolumeGrid::positionOfNodeIndex(uint64_t i) const {
  glm::uvec3 inds = unflattenNodeIndex(i);
  return positionOfNodeIndex(inds);
}

inline glm::vec3 VolumeGrid::positionOfNodeIndex(glm::uvec3 inds) const {
  glm::vec3 tVals = glm::vec3(inds) / glm::vec3(gridNodeDim - 1u);
  return (1.f - tVals) * boundMin + tVals * boundMax;
}

inline uint64_t VolumeGrid::flattenCellIndex(glm::uvec3 inds) const {
  return static_cast<uint64_t>(gridCellDim[0]) * gridCellDim[1] * inds.z + gridCellDim[0] * inds.y + inds.x;
}

inline glm::uvec3 VolumeGrid::unflattenCellIndex(uint64_t i) const {
  uint64_t nXY = gridCellDim[0] * gridCellDim[1];
  uint64_t iZ = i / nXY;
  i -= iZ * nXY;
  uint64_t nX = gridCellDim[0];
  uint64_t iY = i / nX;
  i -= iY * nX;
  uint64_t iX = i;
  return glm::uvec3{static_cast<uint32_t>(iX), static_cast<uint32_t>(iY), static_cast<uint32_t>(iZ)};
}

inline glm::vec3 VolumeGrid::positionOfCellIndex(uint64_t i) const {
  glm::uvec3 inds = unflattenCellIndex(i);
  return positionOfCellIndex(inds);
}

inline glm::vec3 VolumeGrid::positionOfCellIndex(glm::uvec3 inds) const {
  glm::vec3 tVals = (glm::vec3(inds) / glm::vec3(gridCellDim));
  return (1.f - tVals) * boundMin + tVals * boundMax + gridSpacing() / 2.f;
}

inline glm::vec3 VolumeGrid::gridSpacing() const {
  glm::vec3 width = boundMax - boundMin;
  glm::vec3 spacing = width / (glm::vec3(gridCellDim));
  return spacing;
}

inline glm::vec3 VolumeGrid::gridSpacingReference() const {
  glm::vec3 refSpacing{1.f / gridCellDim.x, 1.f / gridCellDim.y, 1.f / gridCellDim.z};
  return refSpacing;
}

inline float VolumeGrid::minGridSpacing() const {
  glm::vec3 spacing = gridSpacing();
  return std::fmin(std::fmin(spacing[0], spacing[1]), spacing[2]);
}

// Shorthand to get a volume grid from polyscope
inline VolumeGrid* getVolumeGrid(std::string name) {
  return dynamic_cast<VolumeGrid*>(getStructure(VolumeGrid::structureTypeName, name));
}
inline bool hasVolumeGrid(std::string name) { return hasStructure(VolumeGrid::structureTypeName, name); }
inline void removeVolumeGrid(std::string name, bool errorIfAbsent) {
  removeStructure(VolumeGrid::structureTypeName, name, errorIfAbsent);
}


// =====================================================
// ============== Quantities
// =====================================================

template <class T>
VolumeGridNodeScalarQuantity* VolumeGrid::addNodeScalarQuantity(std::string name, const T& values, DataType dataType_) {
  validateSize(values, nNodes(), "grid node scalar quantity " + name);
  return addNodeScalarQuantityImpl(name, standardizeArray<float, T>(values), dataType_);
}


template <class Func>
VolumeGridNodeScalarQuantity* VolumeGrid::addNodeScalarQuantityFromCallable(std::string name, Func&& func,
                                                                            DataType dataType_) {

  // Boostrap off the batch version
  auto batchFunc = [&](float* pos_ptr, float* result_ptr, size_t N) {
    for (size_t i = 0; i < N; i++) {
      glm::vec3 pos{pos_ptr[3 * i + 0], pos_ptr[3 * i + 1], pos_ptr[3 * i + 2]};
      result_ptr[i] = func(pos);
    }
  };

  return addNodeScalarQuantityFromBatchCallable(name, batchFunc, dataType_);
}


template <class Func>
VolumeGridNodeScalarQuantity* VolumeGrid::addNodeScalarQuantityFromBatchCallable(std::string name, Func&& func,
                                                                                 DataType dataType_) {
  // Build list of points to query
  std::vector<float> queries(3 * nNodes());
  std::vector<float> result(nNodes());

  // Sample to grid
  for (size_t i = 0; i < nNodes(); i++) {
    glm::vec3 pos = positionOfNodeIndex(i);
    queries[3 * i + 0] = pos.x;
    queries[3 * i + 1] = pos.y;
    queries[3 * i + 2] = pos.z;
  }

  func(&queries.front(), &result.front(), static_cast<size_t>(nNodes()));

  return addNodeScalarQuantity(name, result, dataType_);
}

template <class T>
VolumeGridCellScalarQuantity* VolumeGrid::addCellScalarQuantity(std::string name, const T& values, DataType dataType_) {
  validateSize(values, nCells(), "grid cell scalar quantity " + name);
  return addCellScalarQuantityImpl(name, standardizeArray<float, T>(values), dataType_);
}


template <class Func>
VolumeGridCellScalarQuantity* VolumeGrid::addCellScalarQuantityFromCallable(std::string name, Func&& func,
                                                                            DataType dataType_) {

  // Boostrap off the batch version
  auto batchFunc = [&](float* pos_ptr, float* result_ptr, size_t N) {
    for (size_t i = 0; i < N; i++) {
      glm::vec3 pos{pos_ptr[3 * i + 0], pos_ptr[3 * i + 1], pos_ptr[3 * i + 2]};
      result_ptr[i] = func(pos);
    }
  };

  return addCellScalarQuantityFromBatchCallable(name, batchFunc, dataType_);
}


template <class Func>
VolumeGridCellScalarQuantity* VolumeGrid::addCellScalarQuantityFromBatchCallable(std::string name, Func&& func,
                                                                                 DataType dataType_) {
  // Build list of points to query
  std::vector<float> queries(3 * nCells());
  std::vector<float> result(nCells());

  // Sample to grid
  for (size_t i = 0; i < nCells(); i++) {
    glm::vec3 pos = positionOfCellIndex(i);
    queries[3 * i + 0] = pos.x;
    queries[3 * i + 1] = pos.y;
    queries[3 * i + 2] = pos.z;
  }

  func(&queries.front(), &result.front(), static_cast<size_t>(nCells()));

  return addCellScalarQuantity(name, result, dataType_);
}


} // namespace polyscope
