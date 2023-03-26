// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "imgui.h"

#include "polyscope/polyscope.h"

#include "polyscope/floating_quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

void FloatingQuantity::buildUI() {

  // NOTE: duplicated here and in the QuantityS<S> version

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
