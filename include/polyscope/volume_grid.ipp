// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

namespace polyscope {

inline uint64_t VolumeGrid::nNodes() const {
  return static_cast<uint64_t>(gridNodeDim.x) * gridNodeDim.y * gridNodeDim.z;
}

inline uint64_t VolumeGrid::nCells() const {
  return static_cast<uint64_t>(gridCellDim.x) * gridCellDim.y * gridCellDim.z;
}


inline glm::uvec3 VolumeGrid::flattenNodeIndex(uint64_t i) const {
  uint64_t nYZ = gridNodeDim[1] * gridNodeDim[2];
  uint64_t iX = i / nYZ;
  i -= iX * nYZ;
  uint64_t nZ = gridNodeDim[2];
  uint64_t iY = i / nZ;
  i -= iY * nZ;
  uint64_t iZ = i;
  return glm::uvec3{static_cast<uint32_t>(iX), static_cast<uint32_t>(iY), static_cast<uint32_t>(iZ)};
}

inline glm::vec3 VolumeGrid::positionOfNodeIndex(uint64_t i) const {
  glm::uvec3 inds = flattenNodeIndex(i);
  return positionOfNodeIndex(inds);
}

inline glm::vec3 VolumeGrid::positionOfNodeIndex(glm::uvec3 inds) const {
  glm::vec3 tVals = glm::vec3(inds) / glm::vec3(gridNodeDim - 1u);
  return (1.f - tVals) * boundMin + tVals * boundMax;
}

inline glm::uvec3 VolumeGrid::flattenCellIndex(uint64_t i) const {
  uint64_t nYZ = gridCellDim[1] * gridCellDim[2];
  uint64_t iX = i / nYZ;
  i -= iX * nYZ;
  uint64_t nZ = gridCellDim[2];
  uint64_t iY = i / nZ;
  i -= iY * nZ;
  uint64_t iZ = i;
  return glm::uvec3{static_cast<uint32_t>(iX), static_cast<uint32_t>(iY), static_cast<uint32_t>(iZ)};
}

inline glm::vec3 VolumeGrid::positionOfCellIndex(uint64_t i) const {
  glm::uvec3 inds = flattenCellIndex(i);
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
VolumeGridScalarQuantity* VolumeGrid::addScalarQuantity(std::string name, const T& values, DataType dataType_) {
  validateSize(values, nNodes(), "grid scalar quantity " + name);
  return addScalarQuantityImpl(name, standardizeArray<double, T>(values), dataType_);
}

template <class Func>
VolumeGridScalarQuantity* VolumeGrid::addScalarQuantityFromCallable(std::string name, Func&& func, DataType dataType_) {

  // Sample to grid
  std::vector<double> values(nNodes());
  for (size_t i = 0; i < values.size(); i++) {
    glm::vec3 pos = positionOfNodeIndex(i);
    values[i] = func(pos.x, pos.y, pos.z);
  }

  return addScalarQuantityImpl(name, values, dataType_);
}


template <class Func>
VolumeGridScalarQuantity* VolumeGrid::addScalarQuantityFromBatchCallable(std::string name, Func&& func,
                                                                         DataType dataType_) {

  // TODO make this API match the other implicit callables

  // Build list of points to query
  std::vector<std::array<double, 3>> queries(nNodes());

  // Sample to grid
  for (size_t i = 0; i < queries.size(); i++) {
    glm::vec3 pos = positionOfNodeIndex(i);
    queries[i][0] = pos.x;
    queries[i][1] = pos.y;
    queries[i][2] = pos.z;
  }

  return addScalarQuantity(name, func(queries), dataType_);
}


template <class T>
VolumeGridVectorQuantity* VolumeGrid::addVectorQuantity(std::string name, const T& vecValues, VectorType dataType_) {
  validateSize(vecValues, nNodes(), "grid vector quantity " + name);
  return addVectorQuantityImpl(name, standardizeArray<glm::vec3, T>(vecValues), dataType_);
}

/*
VolumeGridScalarIsosurface* VolumeGrid::addGridIsosurfaceQuantity(std::string name, double isoLevel, const T& values) {
  validateSize(values, nNodes(), "grid isosurface quantity " + name);
  return addIsosurfaceQuantityImpl(name, isoLevel, standardizeArray<double, T>(values));
}

template <class Funct>
VolumeGridScalarQuantity* VolumeGrid::addGridScalarQuantityFromFunction(std::string name, const Funct& funct,
                                                                        DataType dataType_) {
  size_t totalValues = nCornersPerSide * nCornersPerSide * nCornersPerSide;
  std::vector<double> field(totalValues);
  marchingcubes::SampleFunctionToGrid(funct, nCornersPerSide, gridCenter, sideLength, field);
  return addGridScalarQuantity(name, field, dataType_);
}

template <class Funct>
VolumeGridVectorQuantity* VolumeGrid::addGridVectorQuantityFromFunction(std::string name, const Funct& funct,
                                                                        VectorType dataType_) {
  size_t totalValues = nCornersPerSide * nCornersPerSide * nCornersPerSide;
  std::vector<glm::vec3> field(totalValues);
  marchingcubes::SampleFunctionToGrid(funct, nCornersPerSide, gridCenter, sideLength, field);
  return addGridVectorQuantity(name, field, dataType_);
}
*/

/*
template <typename Implicit>
VolumeGrid* registerIsosurfaceFromFunction(std::string name, const Implicit& funct, size_t nValuesPerSide,
                                           glm::vec3 center, double sideLen, bool meshImmediately = true) {

  VolumeGrid* outputSurface = registerVolumeGrid(name, nValuesPerSide, center, sideLen);
  outputSurface->addGridIsosurfaceQuantityFromFunction("isosurface", 0, funct);
  return outputSurface;
}
*/

} // namespace polyscope
