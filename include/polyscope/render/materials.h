// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <array>
#include <iostream>
#include <map>
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
  bool supportsRGB = false;
  std::array<std::shared_ptr<TextureBuffer>, 4> textureBuffers;
};

// Build an ImGui option picker in a dropdown ui
// Returns true if modified.
bool buildMaterialOptionsGui(std::string& mat);


// Read pre-defined materials in to textures
void loadDefaultMaterials();

} // namespace render


// Load new materials from file
void loadBlendableMaterial(std::string matName, std::array<std::string, 4> filenames);
void loadBlendableMaterial(std::string matName, std::string filenameBase, std::string filenameExt);
void loadStaticMaterial(std::string matName, std::string filename);


} // namespace polyscope
