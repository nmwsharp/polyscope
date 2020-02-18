#include "polyscope/volumetric_grid_vector_quantity.h"

#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/vector_shaders.h"

namespace polyscope {

VolumetricGridVectorQuantity::VolumetricGridVectorQuantity(std::string name, VolumetricGrid& grid_,
                                                           const std::vector<glm::vec3>& values_,
                                                           VectorType vectorType_)
    : VolumetricGridQuantity(name, grid_, true), vectorValues(std::move(values_)) {
  vectorType = vectorType_;
  vectorRadius = (vectorType == VectorType::AMBIENT) ? 1.0f : 0.02f;
  vectorLengthMult = 0.2f;

  fillPositions();

  // Create a mapper (default mapper is identity)
  if (vectorType == VectorType::AMBIENT) {
    mapper.setMinMax(vectorValues);
  } else {
    mapper = AffineRemapper<glm::vec3>(vectorValues, DataType::MAGNITUDE);
  }
}

void VolumetricGridVectorQuantity::fillPositions() {
  positions.clear();
  // Fill the positions vector with each grid corner position
  size_t nValues = parent.nValues();
  positions.resize(nValues);
  for (size_t i = 0; i < nValues; i++) {
    positions[i] = parent.positionOfIndex(i);
  }
}

void VolumetricGridVectorQuantity::buildCustomUI() {
  ImGui::SliderFloat("Length", &vectorLengthMult, 0.0, 0.5, "%.5f", 3.);
  ImGui::SliderFloat("Radius", &vectorRadius, 0.0, 0.5, "%.5f", 3.);

  ImGui::TextUnformatted(mapper.printBounds().c_str());
}

std::string VolumetricGridVectorQuantity::niceName() { return name + " (vector)"; }

void VolumetricGridVectorQuantity::geometryChanged() { vectorsProgram.reset(); }

void VolumetricGridVectorQuantity::draw() {
  if (!isEnabled()) return;

  if (vectorsProgram == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*vectorsProgram);

  vectorsProgram->setUniform("u_radius", vectorRadius);
  vectorsProgram->setUniform("u_color", parent.getColor());

  if (vectorType == VectorType::AMBIENT) {
    vectorsProgram->setUniform("u_lengthMult", 1.0);
  } else {
    vectorsProgram->setUniform("u_lengthMult", vectorLengthMult);
  }

  vectorsProgram->draw();
}

void VolumetricGridVectorQuantity::createProgram() {
  vectorsProgram.reset(new gl::GLProgram(&gl::PASSTHRU_VECTOR_VERT_SHADER, &gl::VECTOR_GEOM_SHADER,
                                         &gl::SHINY_VECTOR_FRAG_SHADER, gl::DrawMode::Points));

  // Fill buffers
  std::vector<glm::vec3> mappedVectors;
  for (glm::vec3 v : vectorValues) {
    mappedVectors.push_back(mapper.map(v));
  }

  vectorsProgram->setAttribute("a_vector", mappedVectors);
  vectorsProgram->setAttribute("a_position", positions);

  setMaterialForProgram(*vectorsProgram, "wax");
}

} // namespace polyscope