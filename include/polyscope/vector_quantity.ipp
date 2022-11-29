namespace polyscope {

template <typename QuantityT>
VectorQuantity<QuantityT>::VectorQuantity(QuantityT& quantity_, const std::vector<glm::vec3>& vectors_,
                                          VectorType vectorType_)
    : quantity(quantity_), vectors(vectors_), vectorType(vectorType_),
      vectorLengthMult(quantity.uniquePrefix() + "#vectorLengthMult",
                       vectorType == VectorType::AMBIENT ? absoluteValue(1.0) : relativeValue(0.02)),
      vectorRadius(quantity.uniquePrefix() + "#vectorRadius", relativeValue(0.0025)),
      vectorColor(quantity.uniquePrefix() + "#vectorColor", getNextUniqueColor()),
      material(quantity.uniquePrefix() + "#material", "clay") {}

template <typename QuantityT>
void VectorQuantity<QuantityT>::buildVectorUI() {
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
void VectorQuantity<QuantityT>::drawVectors() {
  if (!vectorProgram) {
    createProgram();
  }

  // Set uniforms
  quantity.parent.setStructureUniforms(*vectorProgram);
  vectorProgram->setUniform("u_radius", vectorRadius.get().asAbsolute());
  vectorProgram->setUniform("u_baseColor", vectorColor.get());

  if (vectorType == VectorType::AMBIENT) {
    vectorProgram->setUniform("u_lengthMult", 1.0);
  } else {
    vectorProgram->setUniform("u_lengthMult", vectorLengthMult.get().asAbsolute() / maxLength);
  }

  glm::mat4 P = view::getCameraPerspectiveMatrix();
  glm::mat4 Pinv = glm::inverse(P);
  vectorProgram->setUniform("u_invProjMatrix", glm::value_ptr(Pinv));
  vectorProgram->setUniform("u_viewport", render::engine->getCurrentViewport());

  vectorProgram->draw();
}

template <typename QuantityT>
void VectorQuantity<QuantityT>::createProgram() {

  std::vector<std::string> rules = quantity.parent.addStructureRules({"SHADE_BASECOLOR"});
  if (quantity.parent.wantsCullPosition()) {
    rules.push_back("VECTOR_CULLPOS_FROM_TAIL");
  }


  // Create the vectorProgram to draw this quantity
  // clang-format off
  vectorProgram = render::engine->requestShader(
      "RAYCAST_VECTOR",
      rules
  );
  // clang-format on

  vectorProgram->setAttribute("a_vector", getVectorRenderBuffer());
  vectorProgram->setAttribute("a_position", baseRenderBuffer);

  render::engine->setMaterial(*vectorProgram, material.get());
}

template <typename QuantityT>
void VectorQuantity<QuantityT>::updateRenderBuffersIfAllocated() {
  if (!vectorRenderBuffer) return;
  vectorRenderBuffer->setData(vectors);
}
template <typename QuantityT>
void VectorQuantity<QuantityT>::ensureRenderBuffersFilled() {
  if (vectorRenderBuffer) return; // if it already exists, quick-out
  vectorRenderBuffer = render::engine->generateAttributeBuffer(RenderDataType::Vector3Float);
  updateRenderBuffersIfAllocated();
}

template <typename QuantityT>
std::shared_ptr<render::AttributeBuffer> VectorQuantity<QuantityT>::getVectorRenderBuffer() {
  ensureRenderBuffersFilled();
  return vectorRenderBuffer;
}

template <typename QuantityT>
void VectorQuantity<QuantityT>::refreshVectors() {
  vectorProgram.reset();
}


template <typename QuantityT>
void VectorQuantity<QuantityT>::dataUpdated() {
  updateRenderBuffersIfAllocated();
  requestRedraw();
}


template <typename QuantityT>
template <class T>
void VectorQuantity<QuantityT>::updateData(const T& newVectors) {
  validateSize(newVectors, vectors.size(), "point cloud vector quantity " + quantity.name);
  vectors = standardizeVectorArray<glm::vec3, 3>(newVectors);
  dataUpdated();
}

template <typename QuantityT>
template <class T>
void VectorQuantity<QuantityT>::updateData2D(const T& newVectors) {
  validateSize(newVectors, vectors.size(), "point cloud vector quantity " + quantity.name);
  vectors = standardizeVectorArray<glm::vec3, 2>(newVectors);
  for (auto& v : vectors) {
    v.z = 0.;
  }
  dataUpdated();
}


template <typename QuantityT>
bool VectorQuantity<QuantityT>::vectorsStoredInMemory() {
  return !vectors.empty();
}

template <typename QuantityT>
size_t VectorQuantity<QuantityT>::nVectorSize() {
  if (vectorsStoredInMemory()) {
    return vectors.size();
  } else {
    if (!vectorRenderBuffer || !vectorRenderBuffer->isSet()) {
      throw std::runtime_error("buffer is not allocated when it should be");
    }
    return static_cast<size_t>(vectorRenderBuffer->getDataSize());
  }
}

template <typename QuantityT>
glm::vec3 VectorQuantity<QuantityT>::getVector(size_t ind) {
  if (vectorsStoredInMemory()) {
    return vectors[ind];
  } else {
    return vectorRenderBuffer->getData_vec3(ind);
  }
}

template <typename QuantityT>
void VectorQuantity<QuantityT>::renderBufferDataExternallyUpdated() {
  vectors.clear();
  requestRedraw();
}

template <typename QuantityT>
QuantityT* VectorQuantity<QuantityT>::setVectorLengthScale(double newLength, bool isRelative) {
  vectorLengthMult = ScaledValue<double>(newLength, isRelative);
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
double VectorQuantity<QuantityT>::getVectorLengthScale() {
  return vectorLengthMult.get().asAbsolute();
}

template <typename QuantityT>
QuantityT* VectorQuantity<QuantityT>::setVectorRadius(double val, bool isRelative) {
  vectorRadius = ScaledValue<double>(val, isRelative);
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
double VectorQuantity<QuantityT>::getVectorRadius() {
  return vectorRadius.get().asAbsolute();
}

template <typename QuantityT>
QuantityT* VectorQuantity<QuantityT>::setVectorColor(glm::vec3 color) {
  vectorColor = color;
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
glm::vec3 VectorQuantity<QuantityT>::getVectorColor() {
  return vectorColor.get();
}

template <typename QuantityT>
QuantityT* VectorQuantity<QuantityT>::setMaterial(std::string m) {
  material = m;
  if (vectorProgram) render::engine->setMaterial(*vectorProgram, getMaterial());
  requestRedraw();
  return &quantity;
}
template <typename QuantityT>
std::string VectorQuantity<QuantityT>::getMaterial() {
  return material.get();
}


} // namespace polyscope
