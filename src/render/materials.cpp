// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/render/materials.h"

#include "polyscope/messages.h"
#include "polyscope/render/engine.h"

#include "stb_image.h"

namespace { // helpers

using namespace polyscope;
using namespace polyscope::render;

BasisMaterial loadBasisMaterial(const std::vector<std::vector<unsigned char>>& data) {

  BasisMaterial newMaterial;

	 std::vector<std::string> names = {"0001.hdr", "0002.hdr", "0003.hdr", "0004.hdr"};

  for (int i = 0; i < 4; i++) {

    /*
        int w, h, comp;
        unsigned char* image = nullptr;
        image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(&data[i][0]), data[i].size(), &w, &h,
       &comp, STBI_rgb); if (image == nullptr) throw std::logic_error("Failed to load material image");

        newMaterial.textureBuffers[i] = engine->generateTextureBuffer(TextureFormat::RGB8, w, h, image);
        stbi_image_free(image);
    */
    int width, height, nrComponents;
    float* data = stbi_loadf(names[i].c_str(), &width, &height, &nrComponents, 0);
    if (!data) {
      polyscope::error("failed to load environment map at " + names[i]);
      continue;
    }

    // Load the texture
    newMaterial.textureBuffers[i] = engine->generateTextureBuffer(TextureFormat::RGB16F, width, height, data);
    stbi_image_free(data);

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
