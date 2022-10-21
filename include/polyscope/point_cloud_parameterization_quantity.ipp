// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

namespace polyscope {

template <class T>
void PointCloudParameterizationQuantity::updateData(const T& newCoords) {
  validateSize(newCoords, coords.size(), "point cloud vector quantity " + name);
  coords = standardizeVectorArray<glm::vec2, 2>(newCoords);
  dataUpdated();
}


} // namespace polyscope
