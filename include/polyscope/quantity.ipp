// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "imgui.h"

#include "polyscope/messages.h"

namespace polyscope {

template <typename S>
Quantity<S>::Quantity(std::string name_, S& parentStructure_, bool dominates_)
    : parent(parentStructure_), name(name_), dominates(dominates_) {}

template <typename S>
Quantity<S>::~Quantity(){};

template <typename S>
void Quantity<S>::draw() {}

template <typename S>
void Quantity<S>::buildUI() {

  if (ImGui::TreeNode(name.c_str())) {

    // Enabled checkbox
    bool enabledLocal = enabled;
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
  return enabled;
}

template <typename S>
void Quantity<S>::setEnabled(bool newEnabled) {
  if (newEnabled == enabled) return;

  enabled = newEnabled;

  // Dominating quantities need to update themselves as their parent's dominating quantity
  if (dominates) {
    if (newEnabled == true) {

      // not too evil here; would only fail if user's program is very broken
      typename S::QuantityType* specificQ = dynamic_cast<typename S::QuantityType*>(this);
      if (specificQ == nullptr) {
        error("tried to set dominant quantity from type which is not QuantityType<ThisStructure>::type");
      }

      parent.setDominantQuantity(specificQ);
    } else {
      parent.clearDominantQuantity();
    }
  }
}

template <typename S>
std::string Quantity<S>::niceName() {
  return name;
}


} // namespace polyscope
