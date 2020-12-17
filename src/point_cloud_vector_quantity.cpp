// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/point_cloud_vector_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include <fstream>
#include <iostream>


using std::cout;
using std::endl;

namespace polyscope {

PointCloudVectorQuantity::PointCloudVectorQuantity(std::string name, std::vector<glm::vec3> vectors_,
                                                   PointCloud& pointCloud_, VectorType vectorType_)

    : PointCloudQuantity(name, pointCloud_), vectors(vectors_), vectorType(vectorType_),
      vectorArtist(new VectorArtist(parent, name + "#vectorartist", parent.points, vectors, vectorType)) {

  if (vectors.size() != parent.points.size()) {
    polyscope::error("Point cloud vector quantity " + name + " does not have same number of values (" +
                     std::to_string(vectors.size()) + ") as point cloud size (" + std::to_string(parent.points.size()) +
                     ")");
  }
}

void PointCloudVectorQuantity::draw() {
  if (!isEnabled()) return;
  vectorArtist->draw();
}

void PointCloudVectorQuantity::refresh() {
  vectorArtist.reset(new VectorArtist(parent, name + "#vectorartist", parent.points, vectors, vectorType));
  Quantity::refresh();
}

void PointCloudVectorQuantity::buildCustomUI() {
  ImGui::SameLine();
  vectorArtist->buildParametersUI();
}

void PointCloudVectorQuantity::buildPickUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectors[ind];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", glm::length(vectors[ind]));
  ImGui::NextColumn();
}

PointCloudVectorQuantity* PointCloudVectorQuantity::setVectorLengthScale(double newLength, bool isRelative) {
  vectorArtist->setVectorLengthScale(newLength, isRelative);
  return this;
}
double PointCloudVectorQuantity::getVectorLengthScale() { return vectorArtist->getVectorLengthScale(); }
PointCloudVectorQuantity* PointCloudVectorQuantity::setVectorRadius(double val, bool isRelative) {
  vectorArtist->setVectorRadius(val, isRelative);
  return this;
}
double PointCloudVectorQuantity::getVectorRadius() { return vectorArtist->getVectorRadius(); }
PointCloudVectorQuantity* PointCloudVectorQuantity::setVectorColor(glm::vec3 color) {
  vectorArtist->setVectorColor(color);
  return this;
}
glm::vec3 PointCloudVectorQuantity::getVectorColor() { return vectorArtist->getVectorColor(); }

PointCloudVectorQuantity* PointCloudVectorQuantity::setMaterial(std::string m) {
  vectorArtist->setMaterial(m);
  return this;
}
std::string PointCloudVectorQuantity::getMaterial() { return vectorArtist->getMaterial(); }

std::string PointCloudVectorQuantity::niceName() { return name + " (vector)"; }

} // namespace polyscope
