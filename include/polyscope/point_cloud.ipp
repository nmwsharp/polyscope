// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

namespace polyscope {


// Shorthand to add a point cloud to polyscope
template <class T>
PointCloud* registerPointCloud(std::string name, const T& points) {
  PointCloud* s = new PointCloud(name, standardizeVectorArray<glm::vec3, 3>(points));
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}
template <class T>
PointCloud* registerPointCloud2D(std::string name, const T& points) {
  std::vector<glm::vec3> points3D(standardizeVectorArray<glm::vec3, 2>(points));
  for (auto& v : points3D) {
    v.z = 0.;
  }
  PointCloud* s = new PointCloud(name, points3D);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

template <class V>
void PointCloud::updatePointPositions(const V& newPositions) {
  points = standardizeVectorArray<glm::vec3, 3>(newPositions);
  geometryChanged();
}

template <class V>
void PointCloud::updatePointPositions2D(const V& newPositions2D) {
  std::vector<glm::vec3> positions3D = standardizeVectorArray<glm::vec3, 2>(newPositions2D);
  for (glm::vec3& v : positions3D) {
    v.z = 0.;
  }

  // Call the main version
  updatePointPositions(positions3D);
}


// Shorthand to get a point cloud from polyscope
inline PointCloud* getPointCloud(std::string name) {
  return dynamic_cast<PointCloud*>(getStructure(PointCloud::structureTypeName, name));
}
inline bool hasPointCloud(std::string name) { return hasStructure(PointCloud::structureTypeName, name); }
inline void removePointCloud(std::string name, bool errorIfAbsent) {
  removeStructure(PointCloud::structureTypeName, name, errorIfAbsent);
}


// =====================================================
// ============== Quantities
// =====================================================


template <class T>
PointCloudColorQuantity* PointCloud::addColorQuantity(std::string name, const T& colors) {
  validateSize(colors, nPoints(), "point cloud color quantity " + name);
  return addColorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(colors));
}


template <class T>
PointCloudScalarQuantity* PointCloud::addScalarQuantity(std::string name, const T& data, DataType type) {
  validateSize(data, nPoints(), "point cloud scalar quantity " + name);
  return addScalarQuantityImpl(name, standardizeArray<double, T>(data), type);
}


template <class T>
PointCloudParameterizationQuantity* PointCloud::addParameterizationQuantity(std::string name, const T& param,
                                                                            ParamCoordsType type) {
  validateSize(param, nPoints(), "point cloud parameterization quantity " + name);
  return addParameterizationQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(param), type);
}

template <class T>
PointCloudParameterizationQuantity* PointCloud::addLocalParameterizationQuantity(std::string name, const T& param,
                                                                                 ParamCoordsType type) {
  validateSize(param, nPoints(), "point cloud parameterization quantity " + name);
  return addLocalParameterizationQuantityImpl(name, standardizeVectorArray<glm::vec2, 2>(param), type);
}

template <class T>
PointCloudVectorQuantity* PointCloud::addVectorQuantity(std::string name, const T& vectors, VectorType vectorType) {
  validateSize(vectors, nPoints(), "point cloud vector quantity " + name);
  return addVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}
template <class T>
PointCloudVectorQuantity* PointCloud::addVectorQuantity2D(std::string name, const T& vectors, VectorType vectorType) {
  validateSize(vectors, nPoints(), "point cloud vector quantity " + name);

  std::vector<glm::vec3> vectors3D(standardizeVectorArray<glm::vec3, 2>(vectors));
  for (auto& v : vectors3D) {
    v.z = 0.;
  }

  return addVectorQuantityImpl(name, vectors3D, vectorType);
}


} // namespace polyscope
