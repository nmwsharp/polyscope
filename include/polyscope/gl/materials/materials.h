#pragma once

#include "polyscope/gl/gl_utils.h"

namespace polyscope {
namespace gl {


// Call once at startup to initialize materials
void loadMaterialTextures();
void unloadMaterialTextures();


// Material textures available.
// Storing as separate index vectors of names and data plays nice with ImGui.

extern std::vector<GLTexturebuffer*> materialTextures;
extern const std::vector<const char*> materialNames;

// Get the index of a material texture (in the materials[] array) by name.
inline int getMaterialIndex(std::string name) {
  for (int i = 0; i < (int)materialNames.size(); i++) {
    if (std::string(materialNames[i]) == name) return i;
  }

  throw std::logic_error("no material with name " + name);
}

inline GLTexturebuffer* getMaterialTexture(std::string name) { return materialTextures[getMaterialIndex(name)]; }

// The arrays that hold the actual data. Stored as constants in translations units in gl/materials/
extern const unsigned char bindata_wax[];
extern const size_t bindata_wax_len;

extern const unsigned char bindata_clay[];
extern const size_t bindata_clay_len;

extern const unsigned char bindata_metal[];
extern const size_t bindata_metal_len;
}
}
