// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/materials.h"

#include "polyscope/messages.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/material_defs.h"

#include "imgui.h"

#include "stb_image.h"

namespace polyscope {
namespace render {

bool buildMaterialOptionsGui(std::string& mat) {
  if (ImGui::BeginMenu("Material")) {
    for (const std::unique_ptr<Material>& o : render::engine->materials) {
      bool selected = (o->name == mat);
      if (ImGui::MenuItem(o->name.c_str(), NULL, selected)) {
        mat = o->name;
        ImGui::EndMenu();
        return true;
      }
    }
    ImGui::EndMenu();
  }
  return false;
}

} // namespace render
} // namespace polyscope
