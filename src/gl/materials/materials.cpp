// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/gl/materials/materials.h"

#include "stb_image.h"

namespace polyscope {
namespace gl {

std::vector<BasisMaterial> materialTextures;

// clang-format off
const std::vector<const char*> materialNames = {
    "wax", 
};
// clang-format on

void loadBasisMaterial(const std::vector<std::vector<unsigned char>>& data) {

  BasisMaterial newMaterial;

  for (int i = 0; i < 3; i++) {

    int w, h, comp;
    unsigned char* image = nullptr;
    image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(&data[i][0]), data[i].size(), &w, &h, &comp,
                                  STBI_rgb);
    if (image == nullptr) throw std::logic_error("Failed to load material image");

    gl::GLTexturebuffer* imageTex = new gl::GLTexturebuffer(GL_RGB, w, h, image);
    newMaterial.textureBuffers[i] = imageTex;
    stbi_image_free(image);
  }

  materialTextures.push_back(newMaterial);
}

void loadMaterialTextures() {

  for (size_t iMat = 0; iMat < materialNames.size(); iMat++) {

    std::string matName = materialNames[iMat];

    if (matName == "wax") {
      loadBasisMaterial(bindata_mat_wax);
    } else {
      std::runtime_error("Unrecognized material name " + matName);
    }
  }
}

void unloadMaterialTextures() {
  for (BasisMaterial& m : materialTextures) {
    for (int i = 0; i < 3; i++) {
      delete m.textureBuffers[i];
    }
  }
}

void setMaterialForProgram(GLProgram& program, std::string name) {
  BasisMaterial material = getMaterialTexture(name);

  program.setTextureFromBuffer("t_mat_r", material.textureBuffers[0]);
  program.setTextureFromBuffer("t_mat_g", material.textureBuffers[1]);
  program.setTextureFromBuffer("t_mat_b", material.textureBuffers[2]);
}

} // namespace gl
} // namespace polyscope
