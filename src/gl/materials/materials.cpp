#include "polyscope/gl/materials/materials.h"

#include "stb_image.h"

namespace polyscope {
namespace gl {

std::vector<GLTexturebuffer*> materialTextures;

// clang-format off
const std::vector<const char*> materialNames = {
    "wax", 
    "metal", 
    "clay",
};
// clang-format on

void loadMaterialTextures() {

  for (size_t iMat = 0; iMat < materialNames.size(); iMat++) {

    std::string matName = materialNames[iMat];

    int w, h, comp;
    unsigned char* image = nullptr;
    if (matName == "wax") {
      image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(bindata_wax), bindata_wax_len, &w, &h, &comp,
                                    STBI_rgb);
    } else if (matName == "metal") {
      image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(bindata_metal), bindata_metal_len, &w, &h,
                                    &comp, STBI_rgb);
    } else if (matName == "clay") {
      image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(bindata_clay), bindata_clay_len, &w, &h,
                                    &comp, STBI_rgb);
    } else {
      std::runtime_error("Unrecognized material name " + matName);
    }

    if (image == nullptr) throw std::logic_error("Failed to load material image " + matName);

    gl::GLTexturebuffer* imageTex = new gl::GLTexturebuffer(GL_RGB, w, h, image);
    materialTextures.push_back(imageTex);

    stbi_image_free(image);
  }
}

void unloadMaterialTextures() {
  for (GLTexturebuffer* t : materialTextures) {
    delete t;
  }
}
}
}
