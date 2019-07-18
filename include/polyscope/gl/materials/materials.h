// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/gl_utils.h"

namespace polyscope {
namespace gl {


// Call once at startup to initialize materials
void loadMaterialTextures();
void unloadMaterialTextures();


// Basis materials have _r, _g, and _b textures for blending with arbitrary surface colors.
struct BasisMaterial {
  std::array<GLTexturebuffer*, 3> textureBuffers;
};


extern std::vector<BasisMaterial> materialTextures;
extern const std::vector<const char*> materialNames;

// Get the index of a material texture (in the materials[] array) by name.
inline int getMaterialIndex(std::string name) {
  for (int i = 0; i < (int)materialNames.size(); i++) {
    if (std::string(materialNames[i]) == name) return i;
  }

  throw std::logic_error("no material with name " + name);
}

inline BasisMaterial getMaterialTexture(std::string name) { return materialTextures[getMaterialIndex(name)]; }

void setMaterialForProgram(GLProgram& program, std::string name);

// The arrays that hold the actual data. Stored as constants in translations units in gl/materials/
extern const std::vector<std::vector<unsigned char>> bindata_mat_wax;
} // namespace gl
} // namespace polyscope
