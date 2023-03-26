// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/volume_grid.h"

#include "imgui.h"

namespace polyscope {

// Initialize statics
const std::string VolumeGrid::structureTypeName = "Volume Grid";

VolumeGrid::VolumeGrid(std::string name, std::array<size_t, 3> steps_, glm::vec3 bound_min_, glm::vec3 bound_max_)
    : QuantityStructure<VolumeGrid>(name, typeName()), steps(steps_), bound_min(bound_min_), bound_max(bound_max_),
      material(uniquePrefix() + "#material", "clay") {
  updateObjectSpaceBounds();
  populateGeometry();
}

void VolumeGrid::buildCustomUI() {
  ImGui::Text("samples: %lld  (%lld, %lld, %lld)", static_cast<long long int>(nValues()),
              static_cast<long long int>(steps[0]), static_cast<long long int>(steps[1]),
              static_cast<long long int>(steps[2]));
  ImGui::TextUnformatted(("min: " + to_string_short(bound_min)).c_str());
  ImGui::TextUnformatted(("max: " + to_string_short(bound_max)).c_str());
}

void VolumeGrid::buildPickUI(size_t localPickID) {
  // For now do nothing
}

void VolumeGrid::draw() {
  // For now, do nothing for the actual grid
  if (!enabled.get()) return;

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->draw();
  }
  for (auto& x : floatingQuantities) {
    x.second->draw();
  }
}

void VolumeGrid::drawDelayed() {
  // For now, do nothing for the actual grid
  if (!enabled.get()) return;

  // Draw the quantities
  for (auto& x : quantities) {
    x.second->drawDelayed();
  }
  for (auto& x : floatingQuantities) {
    x.second->drawDelayed();
  }
}

void VolumeGrid::drawPick() {
  // For now do nothing
}

void VolumeGrid::updateObjectSpaceBounds() {
  objectSpaceBoundingBox = std::make_tuple(bound_min, bound_max);
  objectSpaceLengthScale = glm::length(bound_max - bound_min);
}

std::string VolumeGrid::typeName() { return structureTypeName; }

void VolumeGrid::refresh() {
  QuantityStructure<VolumeGrid>::refresh(); // call base class version, which refreshes quantities
}


void VolumeGrid::populateGeometry() {
  gridPointLocations.resize(nValues());
  for (size_t i = 0; i < gridPointLocations.size(); i++) {
    gridPointLocations[i] = positionOfIndex(i);
  }
}


void VolumeGrid::setVolumeGridUniforms(render::ShaderProgram& p) {}

void VolumeGrid::setVolumeGridPointUniforms(render::ShaderProgram& p) {
  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  p.setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  p.setUniform("u_viewport", render::engine->getCurrentViewport());
  float pointRadius = minGridSpacing() / 8;
  p.setUniform("u_pointRadius", pointRadius);
}


std::vector<std::string> VolumeGrid::addVolumeGridPointRules(std::vector<std::string> initRules) {
  initRules = addStructureRules(initRules);
  if (wantsCullPosition()) {
    initRules.push_back("SPHERE_CULLPOS_FROM_CENTER");
  }
  return initRules;
}

VolumeGrid* VolumeGrid::setMaterial(std::string m) {
  material = m;
  refresh();
  requestRedraw();
  return this;
}
std::string VolumeGrid::getMaterial() { return material.get(); }

VolumeGridQuantity::VolumeGridQuantity(std::string name_, VolumeGrid& curveNetwork_, bool dominates_)
    : QuantityS<VolumeGrid>(name_, curveNetwork_, dominates_) {}


VolumeGridScalarQuantity* VolumeGrid::addScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                            DataType dataType_) {
  VolumeGridScalarQuantity* q = new VolumeGridScalarQuantity(name, *this, data, dataType_);
  addQuantity(q);
  return q;
}

/*
VolumeGridVectorQuantity* VolumeGrid::addVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& data,
                                                            VectorType dataType_) {
  VolumeGridVectorQuantity* q = new VolumeGridVectorQuantity(name, *this, data, dataType_);
  addQuantity(q);
  return q;
}
*/

/*
VolumeGridScalarIsosurface* VolumeGrid::addIsosurfaceQuantityImpl(std::string name, double isoLevel,
                                                                  const std::vector<double>& data) {
  VolumeGridScalarIsosurface* q = new VolumeGridScalarIsosurface(name, *this, data, isoLevel);
  addQuantity(q);
  q->setEnabled(true);
  return q;
}
*/

void VolumeGridQuantity::buildPointInfoGUI(size_t vInd) {}

VolumeGrid* registerVolumeGrid(std::string name, std::array<size_t, 3> steps, glm::vec3 bound_min,
                               glm::vec3 bound_max) {
  VolumeGrid* s = new VolumeGrid(name, steps, bound_min, bound_max);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

VolumeGrid* registerVolumeGrid(std::string name, size_t steps, glm::vec3 bound_min, glm::vec3 bound_max) {
  return registerVolumeGrid(name, {steps, steps, steps}, bound_min, bound_max);
}

} // namespace polyscope
