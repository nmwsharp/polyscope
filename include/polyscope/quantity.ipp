#include "imgui.h"

namespace polyscope {

template <typename S>
Quantity<S>::Quantity(std::string name_, S& parentStructure_, bool dominates_)
    : parent(parentStructure_), name(name_), dominates(dominates_) {}

template <typename S>
Quantity<S>::~Quantity(){};

template <typename S>
void Quantity<S>::drawUI() {

  if (ImGui::TreeNode(name.c_str())) {

    // Enabled checkbox
    bool enabledLocal = enabled;
    ImGui::Checkbox("Enabled", &enabledLocal);
    setEnabled(enabledLocal);
    ImGui::SameLine();

    // Call custom UI
    this->drawCustomUI();

    ImGui::TreePop();
  }
}

template <typename S>
void Quantity<S>::drawCustomUI() {}

template <typename S>
bool Quantity<S>::isEnabled() {
  return enabled;
}

template <typename S>
void Quantity<S>::setEnabled(bool newEnabled) {
  if (newEnabled == enabled) return;

  // Dominating quantities need to update themselves as their parent's dominating quantity
  if (dominates) {
    if (newEnabled == true) {
      parent.setDominantQuantity(this);
    } else {
      parent.clearDominantQuantity();
    }
  }

  enabled = newEnabled;
}

template <typename S>
std::string Quantity<S>::niceName() {
  return name;
}

} // namespace polyscope
