// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run


#include "polyscope/standardize_data_array.h"

namespace polyscope {

// ================================================
// === Base Vector Quantity
// ================================================

template <typename QuantityT>
VectorQuantityBase<QuantityT>::VectorQuantityBase(QuantityT& quantity_, VectorType vectorType_)
    : quantity(quantity_), vectorType(vectorType_),
      vectorLengthMult(quantity.uniquePrefix() + "#vectorLengthMult",
                       vectorType == VectorType::AMBIENT ? absoluteValue(1.0) : relativeValue(0.02)),
      vectorRadius(quantity.uniquePrefix() + "#vectorRadius", relativeValue(0.0025)),
      vectorColor(quantity.uniquePrefix() + "#vectorColor", getNextUniqueColor()),
      material(quantity.uniquePrefix() + "#material", "clay") {}

template <typename QuantityT>
void VectorQuantityBase<QuantityT>::buildVectorUI() {
  ImGui::SameLine();

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
    if (ImGui::SliderFloat("Length", vectorLengthMult.get().getValuePtr(), 0.0, .1, "%.5f",
                           ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
      vectorLengthMult.manuallyChanged();
      requestRedraw();
    }
  }

  if (ImGui::SliderFloat("Radius", vectorRadius.get().getValuePtr(), 0.0, .1, "%.5f",
                         ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat)) {
    vectorRadius.manuallyChanged();
    requestRedraw();
  }

  //{ // Draw max and min magnitude
  // ImGui::TextUnformatted(mapper.printBounds().c_str());
  //}
}


template <typename QuantityT>
QuantityT* VectorQuantityBase<QuantityT>::setVectorLengthScale(double newLength, bool isRelative) {
  vectorLengthMult = ScaledValue<double>(newLength, isRelative);
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
double VectorQuantityBase<QuantityT>::getVectorLengthScale() {
  return vectorLengthMult.get().asAbsolute();
}

template <typename QuantityT>
QuantityT* VectorQuantityBase<QuantityT>::setVectorLengthRange(double newLength) {
  vectorLengthRange = newLength;
  vectorLengthRangeManuallySet = true;
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
double VectorQuantityBase<QuantityT>::getVectorLengthRange() {
  return vectorLengthRange;
}

template <typename QuantityT>
QuantityT* VectorQuantityBase<QuantityT>::setVectorRadius(double val, bool isRelative) {
  vectorRadius = ScaledValue<double>(val, isRelative);
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
double VectorQuantityBase<QuantityT>::getVectorRadius() {
  return vectorRadius.get().asAbsolute();
}

template <typename QuantityT>
QuantityT* VectorQuantityBase<QuantityT>::setVectorColor(glm::vec3 color) {
  vectorColor = color;
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
glm::vec3 VectorQuantityBase<QuantityT>::getVectorColor() {
  return vectorColor.get();
}

template <typename QuantityT>
QuantityT* VectorQuantityBase<QuantityT>::setMaterial(std::string m) {
  material = m;
  if (vectorProgram) render::engine->setMaterial(*vectorProgram, getMaterial());
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
std::string VectorQuantityBase<QuantityT>::getMaterial() {
  return material.get();
}

// ================================================
// === (3D) Vector Quantity
// ================================================

template <typename QuantityT>
VectorQuantity<QuantityT>::VectorQuantity(QuantityT& quantity_, const std::vector<glm::vec3>& vectors_,
                                          render::ManagedBuffer<glm::vec3>& vectorRoots_, VectorType vectorType_)
    : VectorQuantityBase<QuantityT>(quantity_, vectorType_), vectors(quantity_.uniquePrefix() + "#values", vectorsData),
      vectorRoots(vectorRoots_), vectorsData(vectors_) {
  this->updateMaxLength();
}

template <typename QuantityT>
void VectorQuantity<QuantityT>::drawVectors() {
  if (!this->vectorProgram) {
    createProgram();
  }

  // Set uniforms
  this->quantity.parent.setStructureUniforms(*(this->vectorProgram));
  this->vectorProgram->setUniform("u_radius", this->vectorRadius.get().asAbsolute());
  this->vectorProgram->setUniform("u_baseColor", this->vectorColor.get());

  if (this->vectorType == VectorType::AMBIENT) {
    this->vectorProgram->setUniform("u_lengthMult", 1.0);
  } else {
    this->vectorProgram->setUniform("u_lengthMult",
                                    this->vectorLengthMult.get().asAbsolute() / this->vectorLengthRange);
  }

  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  this->vectorProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  this->vectorProgram->setUniform("u_viewport", render::engine->getCurrentViewport());

  this->vectorProgram->draw();
}

template <typename QuantityT>
void VectorQuantity<QuantityT>::createProgram() {

  std::vector<std::string> rules = this->quantity.parent.addStructureRules({"SHADE_BASECOLOR"});
  if (this->quantity.parent.wantsCullPosition()) {
    rules.push_back("VECTOR_CULLPOS_FROM_TAIL");
  }


  // Create the vectorProgram to draw this quantity
  // clang-format off
  this->vectorProgram = render::engine->requestShader(
      "RAYCAST_VECTOR",
      rules
  );
  // clang-format on

  this->vectorProgram->setAttribute("a_vector", vectors.getRenderAttributeBuffer());
  this->vectorProgram->setAttribute("a_position", vectorRoots.getRenderAttributeBuffer());

  render::engine->setMaterial(*(this->vectorProgram), this->material.get());
}

template <typename QuantityT>
void VectorQuantity<QuantityT>::updateMaxLength() {
  if (this->vectorLengthRangeManuallySet) return; // do nothing if it has already been set manually

  vectors.ensureHostBufferPopulated();
  float maxLength = 0.;
  for (const glm::vec3& vec : vectors.data) {
    maxLength = std::max(maxLength, glm::length(vec));
  }
  this->vectorLengthRange = maxLength;
}

template <typename QuantityT>
void VectorQuantity<QuantityT>::refreshVectors() {
  this->vectorProgram.reset();
}

template <typename QuantityT>
template <class T>
void VectorQuantity<QuantityT>::updateData(const T& newVectors) {
  validateSize(newVectors, this->vectors.size(), "vector quantity " + this->quantity.name);
  this->vectors.data = standardizeVectorArray<glm::vec3, 3>(newVectors);
  this->vectors.markHostBufferUpdated();
  this->updateMaxLength();
}

template <typename QuantityT>
template <class T>
void VectorQuantity<QuantityT>::updateData2D(const T& newVectors) {
  validateSize(newVectors, this->vectors.size(), "vector quantity " + this->quantity.name);
  this->vectors.data = standardizeVectorArray<glm::vec3, 2>(newVectors);
  for (auto& v : this->vectors.data) {
    v.z = 0.;
  }
  this->vectors.markHostBufferUpdated();
  this->updateMaxLength();
}

// ================================================
// === Tangent Vector Quantity
// ================================================

template <typename QuantityT>
TangentVectorQuantity<QuantityT>::TangentVectorQuantity(QuantityT& quantity_,
                                                        const std::vector<glm::vec2>& tangentVectors_,
                                                        const std::vector<glm::vec3>& tangentBasisX_,
                                                        const std::vector<glm::vec3>& tangentBasisY_,
                                                        render::ManagedBuffer<glm::vec3>& vectorRoots_, int nSym_,
                                                        VectorType vectorType_)

    : VectorQuantityBase<QuantityT>(quantity_, vectorType_),
      tangentVectors(quantity_.uniquePrefix() + "#values", tangentVectorsData),
      tangentBasisX(quantity_.uniquePrefix() + "#basisX", tangentBasisXData),
      tangentBasisY(quantity_.uniquePrefix() + "#basisY", tangentBasisYData), vectorRoots(vectorRoots_),
      tangentVectorsData(tangentVectors_), tangentBasisXData(tangentBasisX_), tangentBasisYData(tangentBasisY_),
      nSym(nSym_) {
  this->updateMaxLength();
}

template <typename QuantityT>
void TangentVectorQuantity<QuantityT>::drawVectors() {
  if (!this->vectorProgram) {
    createProgram();
  }

  for (int iSym = 0; iSym < nSym; iSym++) { // for drawing symmetric vectors, does nothing in the common case nSym == 1

    float symRotRad = (iSym * 2. * PI) / nSym;
    this->vectorProgram->setUniform("u_vectorRotRad", symRotRad);

    // Set uniforms
    this->quantity.parent.setStructureUniforms(*(this->vectorProgram));
    this->vectorProgram->setUniform("u_radius", this->vectorRadius.get().asAbsolute());
    this->vectorProgram->setUniform("u_baseColor", this->vectorColor.get());

    if (this->vectorType == VectorType::AMBIENT) {
      this->vectorProgram->setUniform("u_lengthMult", 1.0);
    } else {
      this->vectorProgram->setUniform("u_lengthMult",
                                      this->vectorLengthMult.get().asAbsolute() / this->vectorLengthRange);
    }

    glm::mat4 P = view::getCameraPerspectiveMatrix();
    glm::mat4 Pinv = glm::inverse(P);
    this->vectorProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
    this->vectorProgram->setUniform("u_viewport", render::engine->getCurrentViewport());

    this->vectorProgram->draw();
  }
}

template <typename QuantityT>
void TangentVectorQuantity<QuantityT>::createProgram() {

  std::vector<std::string> rules = this->quantity.parent.addStructureRules({"SHADE_BASECOLOR"});
  if (this->quantity.parent.wantsCullPosition()) {
    rules.push_back("VECTOR_CULLPOS_FROM_TAIL");
  }

  // Create the vectorProgram to draw this quantity
  // clang-format off
  this->vectorProgram = render::engine->requestShader(
      "RAYCAST_TANGENT_VECTOR",
      rules
  );
  // clang-format on

  this->vectorProgram->setAttribute("a_tangentVector", tangentVectors.getRenderAttributeBuffer());
  this->vectorProgram->setAttribute("a_basisVectorX", tangentBasisX.getRenderAttributeBuffer());
  this->vectorProgram->setAttribute("a_basisVectorY", tangentBasisY.getRenderAttributeBuffer());
  this->vectorProgram->setAttribute("a_position", vectorRoots.getRenderAttributeBuffer());

  render::engine->setMaterial(*(this->vectorProgram), this->material.get());
}

template <typename QuantityT>
void TangentVectorQuantity<QuantityT>::updateMaxLength() {
  if (this->vectorLengthRangeManuallySet) return; // do nothing if it has already been set manually

  tangentVectors.ensureHostBufferPopulated();
  float maxLength = 0.;
  for (const glm::vec2& vec : tangentVectors.data) {
    maxLength = std::max(maxLength, glm::length(vec));
  }
  this->vectorLengthRange = maxLength;
}

template <typename QuantityT>
void TangentVectorQuantity<QuantityT>::refreshVectors() {
  this->vectorProgram.reset();
}

template <typename QuantityT>
template <class T>
void TangentVectorQuantity<QuantityT>::updateData(const T& newVectors) {
  validateSize(newVectors, this->tangentVectors.size(), "tangent vector quantity " + this->quantity.name);
  this->tangentVectors.data = standardizeVectorArray<glm::vec2, 2>(newVectors);
  this->tangentVectors.markHostBufferUpdated();
  this->updateMaxLength();
}

} // namespace polyscope
