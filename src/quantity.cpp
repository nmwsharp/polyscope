// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/quantity.h"

#include "imgui.h"

#include "polyscope/messages.h"
#include "polyscope/polyscope.h"
#include "polyscope/structure.h"

namespace polyscope {

// === General Quantities
// (subclasses could be a structure-specific quantity or a floating quantity)

Quantity::Quantity(std::string name_, Structure& parentStructure_, bool dominates_)
    : parent(parentStructure_), name(name_), enabled(uniquePrefix() + "enabled", false), dominates(dominates_) {
  validateName(name);

  // Hack: if the quantity pulls enabled=true from the cache, need to make sure the logic from setEnabled(true) happens,
  // so toggle it real quick
  if (isEnabled()) {
    setEnabled(false);
    setEnabled(true);
  }
}

Quantity::~Quantity() {};

void Quantity::draw() {}

void Quantity::drawDelayed() {}

void Quantity::drawPick() {}

void Quantity::drawPickDelayed() {}

void Quantity::buildUI() {
  // NOTE: duplicated here and in the FloatingQuantity version

  if (ImGui::TreeNode(niceName().c_str())) {

    // Enabled checkbox
    bool enabledLocal = enabled.get();
    if (ImGui::Checkbox("Enabled", &enabledLocal)) {
      setEnabled(enabledLocal);
    }

    // Call custom UI
    this->buildCustomUI();

    ImGui::TreePop();
  }
}


void Quantity::buildCustomUI() {}

void Quantity::buildPickUI(size_t localPickInd) {}

bool Quantity::isEnabled() { return enabled.get(); }

void Quantity::setEnabled(bool newEnabled) {
  if (newEnabled == enabled.get()) return;

  enabled = newEnabled;

  // Dominating quantities need to update themselves as their parent's dominating quantity
  if (dominates) {
    if (newEnabled == true) {
      parent.setDominantQuantity(this);
    } else {
      parent.clearDominantQuantity();
    }
  }

  requestRedraw();
}

void Quantity::refresh() { requestRedraw(); }

std::string Quantity::niceName() { return name; }

std::string Quantity::uniquePrefix() { return parent.uniquePrefix() + name + "#"; }

} // namespace polyscope
