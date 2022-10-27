// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/point_cloud_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

namespace polyscope {

PointCloudVectorQuantity::PointCloudVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                   PointCloud& pointCloud_, VectorType vectorType_)

    : PointCloudQuantity(name, pointCloud_), VectorQuantity(*this, vectors_, vectorType_) {

  if (vectors.size() != parent.points.size()) {
    polyscope::error("Point cloud vector quantity " + name + " does not have same number of values (" +
                     std::to_string(vectors.size()) + ") as point cloud size (" + std::to_string(parent.points.size()) +
                     ")");
  }
}

void PointCloudVectorQuantity::draw() {
  if (!isEnabled()) return;

  // ensure that the base buffer has been populated from the parent
  ensureBaseRenderBufferResolved();

  drawVectors();
}

void PointCloudVectorQuantity::ensureBaseRenderBufferResolved() {
  if (!baseRenderBuffer) {
    baseRenderBuffer = parent.getPositionRenderBuffer();
  }
}

void PointCloudVectorQuantity::refresh() {
  refreshVectors();
  Quantity::refresh();
}

void PointCloudVectorQuantity::buildCustomUI() { buildVectorUI(); }

void PointCloudVectorQuantity::buildPickUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  glm::vec3 vec = getVector(ind);
  buffer << vec;
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vec));
  ImGui::NextColumn();
}


std::string PointCloudVectorQuantity::niceName() { return name + " (vector)"; }

} // namespace polyscope
