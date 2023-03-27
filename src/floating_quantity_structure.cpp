// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

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

FloatingQuantityStructure::~FloatingQuantityStructure() {}


void FloatingQuantityStructure::buildCustomUI() {}

void FloatingQuantityStructure::buildCustomOptionsUI() {}

void FloatingQuantityStructure::draw() {
  if (!isEnabled()) return;

  for (auto& qp : quantities) {
    qp.second->draw();
  }
  for (auto& qp : floatingQuantities) {
    qp.second->draw();
  }
}

void FloatingQuantityStructure::drawDelayed() {
  if (!isEnabled()) return;

  for (auto& qp : quantities) {
    qp.second->drawDelayed();
  }
  for (auto& qp : floatingQuantities) {
    qp.second->drawDelayed();
  }
}

void FloatingQuantityStructure::drawPick() {}

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


void FloatingQuantityStructure::buildPickUI(size_t localPickID) {}

// since hasExtents is false, the length scale and bbox value should never be used
bool FloatingQuantityStructure::hasExtents() { return false; }
void FloatingQuantityStructure::updateObjectSpaceBounds() {
  objectSpaceLengthScale = std::numeric_limits<double>::quiet_NaN();
  float nan = std::numeric_limits<float>::quiet_NaN();
  objectSpaceBoundingBox = std::make_tuple(glm::vec3{nan, nan, nan}, glm::vec3{nan, nan, nan});
}

std::string FloatingQuantityStructure::typeName() { return structureTypeName; }

FloatingQuantityStructure* getGlobalFloatingQuantityStructure() {
  if (!internal::globalFloatingQuantityStructure) {
    internal::globalFloatingQuantityStructure = new FloatingQuantityStructure("global");
    bool success = registerStructure(internal::globalFloatingQuantityStructure);
    if (!success) {
      safeDelete(internal::globalFloatingQuantityStructure);
    }
  }
  return internal::globalFloatingQuantityStructure;
}


void removeFloatingQuantityStructureIfEmpty() {
  if (internal::globalFloatingQuantityStructure && internal::globalFloatingQuantityStructure->quantities.empty()) {
    internal::globalFloatingQuantityStructure->remove();
    internal::globalFloatingQuantityStructure = nullptr;
  }
}

void removeFloatingQuantity(std::string name, bool errorIfAbsent) {
  if (!internal::globalFloatingQuantityStructure) {
    if (errorIfAbsent) {
      exception("No floating quantity named " + name + " added.");
    }
    return;
  }

  internal::globalFloatingQuantityStructure->removeQuantity(name, errorIfAbsent);
}

void removeAllFloatingQuantities() {
  if (!internal::globalFloatingQuantityStructure) return;
  internal::globalFloatingQuantityStructure->removeAllQuantities();
}

// Quantity default methods
FloatingQuantity::FloatingQuantity(std::string name_, Structure& parent_) : Quantity(name_, parent_) {}

} // namespace polyscope
