// Copyright 2017-2020, Christopher Yu, Nicholas Sharp and the
// Polyscope contributors. http://polyscope.run.

#include "polyscope/volumetric_grid.h"
#include "polyscope/volumetric_grid_scalar_isosurface.h"

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

VolumetricGrid::VolumetricGrid(std::string name, const std::vector<double>& f, size_t nValuesPerSide, glm::vec3 center,
                               double sideLen)
    : QuantityStructure<VolumetricGrid>(name, typeName()), field(nValuesPerSide * nValuesPerSide * nValuesPerSide),
      color(uniquePrefix() + "#color", getNextUniqueColor()) {
  nCornersPerSide = nValuesPerSide;
  sideLength = sideLen;
  gridCenter = center;
  levelSet = 0;

  int nPerSlice = nCornersPerSide * nCornersPerSide;

  for (size_t x = 0; x < nCornersPerSide; x++) {
    for (size_t y = 0; y < nCornersPerSide; y++) {
      for (size_t z = 0; z < nCornersPerSide; z++) {
        int index = nPerSlice * z + nCornersPerSide * y + x;
        field[index] = f[index];
      }
    }
  }
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

  ImGui::PushItemWidth(100);
  ImGui::InputDouble("level set", &levelSet);
  ImGui::PopItemWidth();
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

} // namespace polyscope