// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/quantity.h"

#include "imgui.h"

#include "polyscope/messages.h"
#include "polyscope/structure.h"

namespace polyscope {

// === General Quantities
// (subclasses could be a structure-specific quantity or a floating quantity)

Quantity::Quantity(std::string name_, Structure& parentStructure_)
    : parent(parentStructure_), name(name_), enabled(parent.typeName() + "#" + parent.name + "#" + name, false) {
  validateName(name);
}

Quantity::~Quantity(){};

void Quantity::draw() {}

void Quantity::drawDelayed() {}

void Quantity::buildUI() {}

void Quantity::buildCustomUI() {}

void Quantity::buildPickUI(size_t localPickInd) {}

bool Quantity::isEnabled() { return enabled.get(); }

void Quantity::refresh() { requestRedraw(); }

std::string Quantity::niceName() { return name; }

std::string Quantity::uniquePrefix() { return parent.uniquePrefix() + name + "#"; }

} // namespace polyscope
