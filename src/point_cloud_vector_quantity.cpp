// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/point_cloud_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

namespace polyscope {

PointCloudVectorQuantity::PointCloudVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                   PointCloud& pointCloud_, VectorType vectorType_)

    : PointCloudQuantity(name, pointCloud_),
      VectorQuantity<PointCloudVectorQuantity>(*this, vectors_, parent.points, vectorType_) {}

void PointCloudVectorQuantity::draw() {
  if (!isEnabled()) return;
  drawVectors();
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
  glm::vec3 vec = vectors.getValue(ind);
  buffer << vec;
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vec));
  ImGui::NextColumn();
}


std::string PointCloudVectorQuantity::niceName() { return name + " (vector)"; }

} // namespace polyscope
