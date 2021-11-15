// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/floating_quantity_structure.h"

#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

namespace polyscope {

// Initialize statics
const std::string FloatingQuantityStructure::structureTypeName = "Floating Quantities";

// Constructor
FloatingQuantityStructure::FloatingQuantityStructure(std::string name)
    : QuantityStructure<FloatingQuantityStructure>(name, structureTypeName) {}


void FloatingQuantityStructure::buildCustomUI() {}

void FloatingQuantityStructure::buildCustomOptionsUI() {}

void FloatingQuantityStructure::draw() {
  for (auto& qp : quantities) {
    qp.second->draw();
  }
}

// override the structure UI, since this one is a bit different
void FloatingQuantityStructure::buildUI() {
  ImGui::PushID(name.c_str()); // ensure there are no conflicts with
                               // identically-named labels


  bool currEnabled = isEnabled();
  ImGui::Checkbox("Enable All", &currEnabled);
  setEnabled(currEnabled);

  // Do any structure-specific stuff here
  this->buildCustomUI();

  // Build quantities list, in the common case of a QuantityStructure
  this->buildQuantitiesUI();

  ImGui::PopID();
}

// since hasExtents is false, the length scale and bbox value should never be used
bool FloatingQuantityStructure::hasExtents() { return false; }
double FloatingQuantityStructure::lengthScale() { return std::numeric_limits<double>::quiet_NaN(); }
std::tuple<glm::vec3, glm::vec3> FloatingQuantityStructure::boundingBox() {
  float nan = std::numeric_limits<float>::quiet_NaN();
  return std::make_tuple(glm::vec3{nan, nan, nan}, glm::vec3{nan, nan, nan});
}

std::string FloatingQuantityStructure::typeName() { return structureTypeName; }


FloatingScalarImageQuantity* FloatingQuantityStructure::addFloatingScalarImageImpl(std::string name, size_t dimX,
                                                                                   size_t dimY,
                                                                                   const std::vector<double>& values,
                                                                                   DataType type) {
  FloatingScalarImageQuantity* q = new FloatingScalarImageQuantity(*this, name, dimX, dimY, values, type);
  addQuantity(q);
  return q;
}
void removeFloatingScalarImage(std::string name) {
  if (!globalFloatingQuantityStructure) return;
  globalFloatingQuantityStructure->removeQuantity(name);
}


FloatingColorImageQuantity* FloatingQuantityStructure::addFloatingColorImageImpl(std::string name, size_t dimX,
                                                                                   size_t dimY,
                                                                                   const std::vector<glm::vec4>& values) {
  FloatingColorImageQuantity* q = new FloatingColorImageQuantity(*this, name, dimX, dimY, values);
  addQuantity(q);
  return q;
}
void removeFloatingColorImage(std::string name) {
  if (!globalFloatingQuantityStructure) return;
  globalFloatingQuantityStructure->removeQuantity(name);
}


FloatingQuantityStructure* getGlobalFloatingQuantityStructure() {
  if (!globalFloatingQuantityStructure) {
    globalFloatingQuantityStructure = new FloatingQuantityStructure("global");
    bool success = registerStructure(globalFloatingQuantityStructure);
    if (!success) {
      safeDelete(globalFloatingQuantityStructure);
    }
  }
  return globalFloatingQuantityStructure;
}

void removeFloatingQuantityStructureIfEmpty() {
  if (globalFloatingQuantityStructure && globalFloatingQuantityStructure->quantities.empty()) {
    globalFloatingQuantityStructure->remove();
    globalFloatingQuantityStructure = nullptr;
  }
}


// Quantity default methods
FloatingQuantity::FloatingQuantity(std::string name_, FloatingQuantityStructure& parent_, bool dominates_)
    : Quantity<FloatingQuantityStructure>(name_, parent_, dominates_) {}

} // namespace polyscope
