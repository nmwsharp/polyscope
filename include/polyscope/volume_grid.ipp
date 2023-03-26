// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

namespace polyscope {

inline size_t VolumeGrid::nValues() const { return steps[0] * steps[1] * steps[2]; }


inline std::array<size_t, 3> VolumeGrid::flattenIndex(size_t i) const {
  size_t nYZ = steps[1] * steps[2];
  size_t iX = i / nYZ;
  i -= iX * nYZ;
  size_t nZ = steps[2];
  size_t iY = i / nZ;
  i -= iY * nZ;
  size_t iZ = i;
  return std::array<size_t, 3>{iX, iY, iZ};
}

inline glm::vec3 VolumeGrid::positionOfIndex(size_t i) const {
  std::array<size_t, 3> inds = flattenIndex(i);
  return positionOfIndex(inds);
}

inline glm::vec3 VolumeGrid::positionOfIndex(std::array<size_t, 3> inds) const {
  glm::vec3 tVals{
      static_cast<float>(inds[0]) / (steps[0] - 1),
      static_cast<float>(inds[1]) / (steps[1] - 1),
      static_cast<float>(inds[2]) / (steps[2] - 1),
  };
  return (1.f - tVals) * bound_min + tVals * bound_max;
}

inline glm::vec3 VolumeGrid::gridSpacing() const {
  glm::vec3 width = bound_max - bound_min;
  glm::vec3 spacing = width / glm::vec3{
                                  steps[0] - 1.f,
                                  steps[1] - 1.f,
                                  steps[2] - 1.f,
                              };
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
  validateSize(values, nValues(), "grid scalar quantity " + name);
  return addScalarQuantityImpl(name, standardizeArray<double, T>(values), dataType_);
}

template <class Func>
VolumeGridScalarQuantity* VolumeGrid::addScalarQuantityFromCallable(std::string name, Func&& func, DataType dataType_) {

  // Sample to grid
  std::vector<double> values(nValues());
  for (size_t i = 0; i < values.size(); i++) {
    glm::vec3 pos = positionOfIndex(i);
    values[i] = func(pos.x, pos.y, pos.z);
  }

  return addScalarQuantityImpl(name, values, dataType_);
}


template <class Func>
VolumeGridScalarQuantity* VolumeGrid::addScalarQuantityFromBatchCallable(std::string name, Func&& func,
                                                                         DataType dataType_) {

  // Build list of points to query
  std::vector<std::array<double, 3>> queries(nValues());

  // Sample to grid
  for (size_t i = 0; i < queries.size(); i++) {
    glm::vec3 pos = positionOfIndex(i);
    queries[i][0] = pos.x;
    queries[i][1] = pos.y;
    queries[i][2] = pos.z;
  }

  return addScalarQuantity(name, func(queries), dataType_);
}


template <class T>
VolumeGridVectorQuantity* VolumeGrid::addVectorQuantity(std::string name, const T& vecValues, VectorType dataType_) {
  validateSize(vecValues, nValues(), "grid vector quantity " + name);
  return addVectorQuantityImpl(name, standardizeArray<glm::vec3, T>(vecValues), dataType_);
}

/*
VolumeGridScalarIsosurface* VolumeGrid::addGridIsosurfaceQuantity(std::string name, double isoLevel, const T& values) {
  validateSize(values, nValues(), "grid isosurface quantity " + name);
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
