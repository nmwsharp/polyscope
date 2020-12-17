// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/vector_artist.h"

#include "imgui.h"


namespace polyscope {

VectorArtist::VectorArtist(Structure& parentStructure_, std::string uniqueName_, const std::vector<glm::vec3>& bases_,
                           const std::vector<glm::vec3>& vectors_, const VectorType& vectorType_)
    : parentStructure(parentStructure_), uniqueName(uniqueName_),
      uniquePrefix(parentStructure.uniquePrefix() + "#" + uniqueName), vectorType(vectorType_), bases(bases_),
      vectors(vectors_), vectorLengthMult(uniquePrefix + "#vectorLengthMult",
                                          vectorType == VectorType::AMBIENT ? absoluteValue(1.0) : relativeValue(0.02)),
      vectorRadius(uniquePrefix + "#vectorRadius", relativeValue(0.0025)),
      vectorColor(uniquePrefix + "#vectorColor", getNextUniqueColor()), material(uniquePrefix + "#material", "clay") {

  updateMaxLength();
}

void VectorArtist::updateMaxLength() {
  // compute the max length
  maxLength = 0.;
  for (const glm::vec3& vec : vectors) {
    double l2 = glm::length2(vec);
    if (!std::isfinite(l2)) continue;
    maxLength = std::fmax(maxLength, l2);
  }
  maxLength = std::sqrt(maxLength);
  if (maxLength == 0.) maxLength = 1e-16;
}

void VectorArtist::draw() {
  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parentStructure.setTransformUniforms(*program);

  program->setUniform("u_radius", vectorRadius.get().asAbsolute());
  program->setUniform("u_baseColor", vectorColor.get());

  if (vectorType == VectorType::AMBIENT) {
    program->setUniform("u_lengthMult", 1.0);
  } else {
    program->setUniform("u_lengthMult", vectorLengthMult.get().asAbsolute() / maxLength);
  }

  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  program->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  program->setUniform("u_viewport", render::engine->getCurrentViewport());

  program->draw();
}

void VectorArtist::createProgram() {
  program = render::engine->requestShader("RAYCAST_VECTOR", {"SHADE_BASECOLOR"});

  // Fill buffers
  program->setAttribute("a_vector", vectors);
  program->setAttribute("a_position", bases);

  render::engine->setMaterial(*program, material.get());
}

void VectorArtist::buildParametersUI() {

  if (ImGui::ColorEdit3("Color", &vectorColor.get()[0], ImGuiColorEditFlags_NoInputs)) setVectorColor(getVectorColor());
  ImGui::SameLine();

  // === Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {
    if (render::buildMaterialOptionsGui(material.get())) {
      material.manuallyChanged();
      setMaterial(material.get()); // trigger the other updates that happen on set()
    }
    ImGui::EndPopup();
  }

  // Only get to set length for non-ambient vectors
  if (vectorType != VectorType::AMBIENT) {
    if (ImGui::SliderFloat("Length", vectorLengthMult.get().getValuePtr(), 0.0, .1, "%.5f", 3.)) {
      vectorLengthMult.manuallyChanged();
      requestRedraw();
    }
  }

  if (ImGui::SliderFloat("Radius", vectorRadius.get().getValuePtr(), 0.0, .1, "%.5f", 3.)) {
    vectorRadius.manuallyChanged();
    requestRedraw();
  }

  //{ // Draw max and min magnitude
    //ImGui::TextUnformatted(mapper.printBounds().c_str());
  //}
}

void VectorArtist::setVectorLengthScale(double newLength, bool isRelative) {
  vectorLengthMult = ScaledValue<double>(newLength, isRelative);
  requestRedraw();
}
double VectorArtist::getVectorLengthScale() { return vectorLengthMult.get().asAbsolute(); }

void VectorArtist::setVectorRadius(double val, bool isRelative) {
  vectorRadius = ScaledValue<double>(val, isRelative);
  requestRedraw();
}
double VectorArtist::getVectorRadius() { return vectorRadius.get().asAbsolute(); }

void VectorArtist::setVectorColor(glm::vec3 color) {
  vectorColor = color;
  requestRedraw();
}
glm::vec3 VectorArtist::getVectorColor() { return vectorColor.get(); }

void VectorArtist::setMaterial(std::string m) {
  material = m;
  if (program) render::engine->setMaterial(*program, getMaterial());
  requestRedraw();
}
std::string VectorArtist::getMaterial() { return material.get(); }


} // namespace polyscope
