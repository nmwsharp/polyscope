// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/materials.h"

#include "polyscope/messages.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/material_defs.h"

#include "imgui.h"

#include "stb_image.h"

namespace { // helpers

using namespace polyscope;
using namespace polyscope::render;

BasisMaterial loadBasisMaterial(Material m) {

  BasisMaterial newMaterial;

  std::array<unsigned char const*, 4> buff;
  std::array<size_t, 4> buffSize;

  // clang-format off
  switch (m) {
  case Material::Clay:
    buff[0] = &bindata_clay_r[0]; buffSize[0] = bindata_clay_r.size();
    buff[1] = &bindata_clay_g[0]; buffSize[1] = bindata_clay_g.size();
    buff[2] = &bindata_clay_b[0]; buffSize[2] = bindata_clay_b.size();
    buff[3] = &bindata_clay_k[0]; buffSize[3] = bindata_clay_k.size();
    break;
  case Material::Wax:
    buff[0] = &bindata_wax_r[0]; buffSize[0] = bindata_wax_r.size();
    buff[1] = &bindata_wax_g[0]; buffSize[1] = bindata_wax_g.size();
    buff[2] = &bindata_wax_b[0]; buffSize[2] = bindata_wax_b.size();
    buff[3] = &bindata_wax_k[0]; buffSize[3] = bindata_wax_k.size();
    break;
  case Material::Candy:
    buff[0] = &bindata_candy_r[0]; buffSize[0] = bindata_candy_r.size();
    buff[1] = &bindata_candy_g[0]; buffSize[1] = bindata_candy_g.size();
    buff[2] = &bindata_candy_b[0]; buffSize[2] = bindata_candy_b.size();
    buff[3] = &bindata_candy_k[0]; buffSize[3] = bindata_candy_k.size();
    break;
  case Material::Flat:
    buff[0] = &bindata_flat_r[0]; buffSize[0] = bindata_flat_r.size();
    buff[1] = &bindata_flat_g[0]; buffSize[1] = bindata_flat_g.size();
    buff[2] = &bindata_flat_b[0]; buffSize[2] = bindata_flat_b.size();
    buff[3] = &bindata_flat_k[0]; buffSize[3] = bindata_flat_k.size();
    break;
  }
  // clang-format on


  /*
      int w, h, comp;
      unsigned char* image = nullptr;
      image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(&data[i][0]), data[i].size(), &w, &h,
     &comp, STBI_rgb); if (image == nullptr) throw std::logic_error("Failed to load material image");

      newMaterial.textureBuffers[i] = engine->generateTextureBuffer(TextureFormat::RGB8, w, h, image);
  newMaterial.textureBuffers[i]->setFilterMode(FilterMode::Linear);
      stbi_image_free(image);
  */

  for (int i = 0; i < 4; i++) {
    int width, height, nrComponents;
    float* data = stbi_loadf_from_memory(buff[i], buffSize[i], &width, &height, &nrComponents, 3);
    if (!data) polyscope::error("failed to load environment map");
    newMaterial.textureBuffers[i] = engine->generateTextureBuffer(TextureFormat::RGB16F, width, height, data);
    newMaterial.textureBuffers[i]->setFilterMode(FilterMode::Linear);
    stbi_image_free(data);
  }

  return newMaterial;
}

} // namespace

namespace polyscope {
namespace render {

std::string name(Material m) {
  switch (m) {
  case Material::Clay:
    return "clay";
  case Material::Wax:
    return "wax";
  case Material::Candy:
    return "candy";
  case Material::Flat:
    return "flat";
  }
  throw std::runtime_error("bad enum");
}

std::map<Material, BasisMaterial> loadDefaultMaterials() {
  std::map<Material, BasisMaterial> d = {
      {Material::Clay, loadBasisMaterial(Material::Clay)},
      {Material::Wax, loadBasisMaterial(Material::Wax)},
      {Material::Candy, loadBasisMaterial(Material::Candy)},
      {Material::Flat, loadBasisMaterial(Material::Flat)},
  };
  return d;
}

bool buildMaterialOptionsGui(Material& m) {
  if (ImGui::BeginMenu("Material")) {
    for (Material o : {Material::Clay, Material::Wax, Material::Candy, Material::Flat}) {
      bool selected = (o == m);
      if (ImGui::MenuItem(name(o).c_str(), NULL, selected)) {
        m = o;
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
