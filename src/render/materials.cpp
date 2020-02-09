// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/materials.h"

#include "polyscope/render/engine.h"

#include "stb_image.h"

namespace { // helpers

using namespace polyscope::render;

BasisMaterial loadBasisMaterial(const std::vector<std::vector<unsigned char>>& data) {

  BasisMaterial newMaterial;

  for (int i = 0; i < 3; i++) {

    int w, h, comp;
    unsigned char* image = nullptr;
    image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(&data[i][0]), data[i].size(), &w, &h, &comp,
                                  STBI_rgb);
    if (image == nullptr) throw std::logic_error("Failed to load material image");

    newMaterial.textureBuffers[i] = engine->generateTextureBuffer(TextureFormat::RGB8, w, h, image);
    stbi_image_free(image);
  }

  return newMaterial;
}

} // namespace

namespace polyscope {
namespace render {

std::string name(Material m) {
  switch (m) {
  case Material::Wax:
    return "wax";
  }
}

std::map<Material, BasisMaterial> loadDefaultMaterials() {
  std::map<Material, BasisMaterial> d = {
      {Material::Wax, loadBasisMaterial(bindata_mat_wax)},
  };
  return d;
}

} // namespace render
} // namespace polyscope
