// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/standardize_data_array.h"

namespace polyscope {

// Encapsulates logic which is common to all scalar quantities

template <typename QuantityT>
class ColorQuantity {
public:
  ColorQuantity(QuantityT& parent, const std::vector<glm::vec3>& colors);

  // Build the ImGUI UIs for scalars
  void buildColorUI();

  // Add rules to rendering programs for scalars
  std::vector<std::string> addColorRules(std::vector<std::string> rules);

  // Set uniforms in rendering programs for scalars
  void setColorUniforms(render::ShaderProgram& p);

  template <class V>
  void updateData(const V& newColors);

  // === Members
  QuantityT& quantity;
  render::ManagedBuffer<glm::vec3> colors;

  // === Get/set visualization parameters

  // === ~DANGER~ experimental/unsupported functions

protected:
  std::vector<glm::vec3> colorsData;

  // === Visualization parameters

  // Parameters
};

} // namespace polyscope


#include "polyscope/color_quantity.ipp"
