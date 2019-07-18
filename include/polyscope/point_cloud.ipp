// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

namespace polyscope {

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
PointCloudVectorQuantity* PointCloud::addVectorQuantity(std::string name, const T& vectors, VectorType vectorType) {
  validateSize(vectors, nPoints(), "point cloud vector quantity " + name);
  return addVectorQuantityImpl(name, standardizeVectorArray<glm::vec3, 3>(vectors), vectorType);
}


} // namespace polyscope
