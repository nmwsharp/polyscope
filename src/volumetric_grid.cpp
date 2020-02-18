// Copyright 2017-2020, Christopher Yu, Nicholas Sharp and the
// Polyscope contributors. http://polyscope.run.

#include "polyscope/volumetric_grid.h"
#include "polyscope/volumetric_grid_scalar_isosurface.h"
#include "polyscope/volumetric_grid_scalar_quantity.h"
#include "polyscope/volumetric_grid_vector_quantity.h"

#include "polyscope/combining_hash_functions.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/gl/shaders/wireframe_shaders.h"
#include "polyscope/pick.h"
#include "polyscope/polyscope.h"

#include "polyscope/surface_mesh.h"

#include "imgui.h"

namespace polyscope {
// Initialize statics
const std::string VolumetricGrid::structureTypeName = "Implicit Surface";

VolumetricGrid::VolumetricGrid(std::string name, size_t nValuesPerSide, glm::vec3 center, double sideLen)
    : QuantityStructure<VolumetricGrid>(name, typeName()), color(uniquePrefix() + "#color", getNextUniqueColor()) {
  nCornersPerSide = nValuesPerSide;
  sideLength = sideLen;
  gridCenter = center;
}

VolumetricGrid* VolumetricGrid::setColor(glm::vec3 newVal) {
  color = newVal;
  polyscope::requestRedraw();
  return this;
}

glm::vec3 VolumetricGrid::getColor() { return color.get(); }

void VolumetricGrid::buildCustomUI() {
  ImGui::Text("samples: %lld  (%lld per side)", static_cast<long long int>(nValues()),
              static_cast<long long int>(nCornersPerSide));
  ImGui::Text("center: (%.3f, %.3f, %.3f)", gridCenter.x, gridCenter.y, gridCenter.z);
  ImGui::Text("grid side length: %.4f", sideLength);
  if (ImGui::ColorEdit3("Color", &color.get()[0], ImGuiColorEditFlags_NoInputs)) {
    setColor(getColor());
  }
}

void VolumetricGrid::buildPickUI(size_t localPickID) {
  // For now do nothing
}

void VolumetricGrid::draw() {
  // For now, do nothing for the actual grid

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }
}

void VolumetricGrid::drawPick() {
  // For now do nothing
}

double VolumetricGrid::lengthScale() { return sideLength; }

std::tuple<glm::vec3, glm::vec3> VolumetricGrid::boundingBox() {
  double r = sideLength / 2;
  glm::vec3 minCorner = gridCenter - glm::vec3{r, r, r};
  glm::vec3 maxCorner = gridCenter + glm::vec3{r, r, r};

  return std::make_tuple(minCorner, maxCorner);
}

std::string VolumetricGrid::typeName() { return structureTypeName; }

VolumetricGridQuantity::VolumetricGridQuantity(std::string name_, VolumetricGrid& curveNetwork_, bool dominates_)
    : Quantity<VolumetricGrid>(name_, curveNetwork_, dominates_) {}


void VolumetricGridQuantity::buildNodeInfoGUI(size_t nodeInd) {}
void VolumetricGridQuantity::buildEdgeInfoGUI(size_t edgeInd) {}

VolumetricGridScalarIsosurface* VolumetricGrid::addIsosurfaceQuantityImpl(std::string name, double isoLevel,
                                                                          const std::vector<double>& data) {
  VolumetricGridScalarIsosurface* q = new VolumetricGridScalarIsosurface(name, *this, data, isoLevel);
  addQuantity(q);
  q->setEnabled(true);
  return q;
}

VolumetricGridScalarQuantity* VolumetricGrid::addScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                                    DataType dataType_) {
  VolumetricGridScalarQuantity* q = new VolumetricGridScalarQuantity(name, *this, data, dataType_);
  addQuantity(q);
  return q;
}

VolumetricGridVectorQuantity* VolumetricGrid::addVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& data,
                                                    VectorType dataType_) {
  VolumetricGridVectorQuantity* q = new VolumetricGridVectorQuantity(name, *this, data, dataType_);
  addQuantity(q);
  return q;
}

VolumetricGrid* registerVolumetricGrid(std::string name, size_t nValuesPerSide, glm::vec3 center, double sideLen) {
  VolumetricGrid* s = new VolumetricGrid(name, nValuesPerSide, center, sideLen);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

} // namespace polyscope