// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

namespace polyscope {

template <class T>
void PointCloudScalarQuantity::updateData(const T& newValues) {
  validateSize(newValues, values.size(), "point cloud scalar quantity " + name);
  values = standardizeArray<double, T>(newValues);
  dataUpdated();
}

} // namespace polyscope
