// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

namespace polyscope {

template <class T>
void PointCloudColorQuantity::updateData(const T& newValues) {
  validateSize(newValues, values.size(), "point cloud color quantity " + name);
  values = standardizeVectorArray<glm::vec3, 3>(newValues);
  dataUpdated();
}

} // namespace polyscope
