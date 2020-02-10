// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <array>
#include <map>
#include <memory>
#include <vector>

namespace polyscope {

// Pull the enum to the outer namespace to keep typing burden down
enum class Material { Clay = 0, Wax, Candy, Flat };

namespace render {

// Give the enums names
std::string name(Material m);

// forward declare
class TextureBuffer;

// Basis materials have _r, _g, _b, _k textures for blending with arbitrary surface colors.
struct BasisMaterial {
  std::array<std::shared_ptr<TextureBuffer>, 4> textureBuffers;
};


// Build an ImGui option picker in a dropdown ui
// Returns true if modified.
bool buildMaterialOptionsGui(Material& m);

// Read pre-defined materials in to textures
std::map<Material, BasisMaterial> loadDefaultMaterials();

} // namespace render
} // namespace polyscope
