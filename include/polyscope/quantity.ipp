// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "imgui.h"

#include "polyscope/messages.h"
#include "polyscope/structure.h"

namespace polyscope {

// forward declaration
void requestRedraw();

// === Structure-specific Quantities

template <typename S>
QuantityS<S>::QuantityS(std::string name_, S& parentStructure_, bool dominates_)
    : Quantity(name_, parentStructure_), parent(parentStructure_), dominates(dominates_) {
  validateName(name);

  // Hack: if the quantity pulls enabled=true from the cache, need to make sure the logic from setEnabled(true) happens,
  // so toggle it real quick
  if (isEnabled()) {
    setEnabled(false);
    setEnabled(true);
  }
}

template <typename S>
QuantityS<S>::~QuantityS() {}

template <typename S>
QuantityS<S>* QuantityS<S>::setEnabled(bool newEnabled) {
  if (newEnabled == enabled.get()) return this;

  enabled = newEnabled;

  // Dominating quantities need to update themselves as their parent's dominating quantity
  if (dominates) {
    if (newEnabled == true) {
      parent.setDominantQuantity(this);
    } else {
      parent.clearDominantQuantity();
    }
  }

  if (isEnabled()) {
    requestRedraw();
  }

  return this;
}

template <typename S>
void QuantityS<S>::buildUI() {
  // NOTE: duplicated here and in the FloatingQuantity version

  if (ImGui::TreeNode(niceName().c_str())) {

    // Enabled checkbox
    bool enabledLocal = enabled.get();
    ImGui::Checkbox("Enabled", &enabledLocal);
    setEnabled(enabledLocal);

    // Call custom UI
    this->buildCustomUI();

    ImGui::TreePop();
  }
}

} // namespace polyscope
