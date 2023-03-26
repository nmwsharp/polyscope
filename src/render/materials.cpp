// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

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
      std::string fancyName = o->name;
      if (o->supportsRGB) {
        fancyName += " (rgb)";
      }
      if (ImGui::MenuItem(fancyName.c_str(), NULL, selected)) {
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

void loadBlendableMaterial(std::string matName, std::array<std::string, 4> filenames) {
  render::engine->loadBlendableMaterial(matName, filenames);
}
void loadBlendableMaterial(std::string matName, std::string filenameBase, std::string filenameExt) {
  render::engine->loadBlendableMaterial(matName, filenameBase, filenameExt);
}
void loadStaticMaterial(std::string matName, std::string filename) {
  render::engine->loadStaticMaterial(matName, filename);
}

} // namespace polyscope
