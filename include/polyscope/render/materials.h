// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <array>
#include <map>
#include <iostream>
#include <memory>
#include <vector>

namespace polyscope {

// == Predefinied materials:
//
// RGB colorable materials:
//   - clay: Simple material without much specularity, default for most object.
//   - wax: Slightly more specular and exciting looking
//   - candy: Shiny and bright, useful for accents
//   - flat: Flat shading, plain RGB lookups
//
// Single-color materials:
//   -

namespace render {

// forward declare
class TextureBuffer;

// Materials have _r, _g, _b, _k textures for blending with arbitrary surface colors.
struct Material {
  std::string name;
  std::array<std::shared_ptr<TextureBuffer>, 4> textureBuffers;
};

// Build an ImGui option picker in a dropdown ui
// Returns true if modified.
bool buildMaterialOptionsGui(std::string& mat);


// Read pre-defined materials in to textures
void loadDefaultMaterials();

} // namespace render
} // namespace polyscope
