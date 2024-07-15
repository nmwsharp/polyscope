#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/standardize_data_array.h"

namespace polyscope {

template <typename QuantityT>
class TetracolorQuantity {
public:
  TetracolorQuantity(QuantityT& parent, const std::vector<glm::vec4>& colors);

  // Add rules to rendering programs for scalars
  std::vector<std::string> addTetracolorRules(std::vector<std::string> rules);

  // Set uniforms in rendering programs for scalars
  void setTetracolorUniforms(render::ShaderProgram& p);

  // === Members
  QuantityT& quantity;
  render::ManagedBuffer<glm::vec4> tetracolors;

protected:
  std::vector<glm::vec4> tetracolorsData;


}; // class TetracolorQuantity

} // namespace polyscope

#include "polyscope/tetracolor_quantity.ipp"
