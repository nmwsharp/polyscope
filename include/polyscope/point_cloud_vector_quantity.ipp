// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

namespace polyscope {

template <class T>
void PointCloudVectorQuantity::updateData(const T& newVectors) {
  validateSize(newVectors, vectors.size(), "point cloud vector quantity " + name);
  vectors = standardizeVectorArray<glm::vec3, 3>(newVectors);
  dataUpdated();
}

template <class T>
void PointCloudVectorQuantity::updateData2D(const T& newVectors) {
  validateSize(newVectors, vectors.size(), "point cloud vector quantity " + name);
  vectors = standardizeVectorArray<glm::vec3, 2>(newVectors);
  for (auto& v : vectors) {
    v.z = 0.;
  }
  dataUpdated();
}

} // namespace polyscope
