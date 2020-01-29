// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "imgui.h"

#include "polyscope/messages.h"

namespace polyscope {

template <typename S>
Quantity<S>::Quantity(std::string name_, S& parentStructure_, bool dominates_)
    : parent(parentStructure_), name(name_), enabled(parent.typeName() + "#" + parent.name + "#" + name, false),
      dominates(dominates_) {
  // Hack: if the quantity pulls enabled=true from the cache, need to make sure the logic from setEnabled(true) happens,
  // so toggle it real quick
  if (isEnabled()) {
    setEnabled(false);
    setEnabled(true);
  }
}

template <typename S>
Quantity<S>::~Quantity(){};

template <typename S>
void Quantity<S>::draw() {}

template <typename S>
void Quantity<S>::buildUI() {

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

template <typename S>
void Quantity<S>::buildCustomUI() {}

template <typename S>
void Quantity<S>::buildPickUI(size_t localPickInd) {}

template <typename S>
bool Quantity<S>::isEnabled() {
  return enabled.get();
}

// forward declaration
void requestRedraw();

template <typename S>
Quantity<S>* Quantity<S>::setEnabled(bool newEnabled) {
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
std::string Quantity<S>::niceName() {
  return name;
}

template <typename S>
std::string Quantity<S>::uniquePrefix() {
  return parent.uniquePrefix() + name + "#";
}

} // namespace polyscope
