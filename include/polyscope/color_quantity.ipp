namespace polyscope {

template <typename QuantityT>
ColorQuantity<QuantityT>::ColorQuantity(QuantityT& quantity_, const std::vector<glm::vec3>& colors_)
    : quantity(quantity_), colors(colors_) {}

template <typename QuantityT>
void ColorQuantity<QuantityT>::buildColorUI() {}

template <typename QuantityT>
void ColorQuantity<QuantityT>::setColorUniforms(render::ShaderProgram& p) {}

template <typename QuantityT>
std::vector<std::string> ColorQuantity<QuantityT>::addColorRules(std::vector<std::string> rules) {
  return rules;
}

template <typename QuantityT>
void ColorQuantity<QuantityT>::ensureRenderBuffersFilled(bool forceRefill) {

  // ## create the buffers if they don't already exist

  bool createdBuffer = false;
  if (!colorRenderBuffer) {
    colorRenderBuffer = render::engine->generateAttributeBuffer(RenderDataType::Vector3Float);
    createdBuffer = true;
  }

  // If the buffers already existed (and thus are presumably filled), quick-out. Otherwise, fill the buffers.
  if (createdBuffer || forceRefill) {
    colorRenderBuffer->setData(colors);
  }
}


template <typename QuantityT>
template <class V>
void ColorQuantity<QuantityT>::updateData(const V& newColors) {
  validateSize(newColors, nColorSize(), "color quantity");
  colors = standardizeVectorArray<glm::vec3, 3>(newColors);
  dataUpdated();
}

template <typename QuantityT>
std::shared_ptr<render::AttributeBuffer> ColorQuantity<QuantityT>::getColorRenderBuffer() {
  ensureRenderBuffersFilled();
  return colorRenderBuffer;
}

template <typename QuantityT>
void ColorQuantity<QuantityT>::dataUpdated() {
  ensureRenderBuffersFilled(false);
  requestRedraw();
}

template <typename QuantityT>
bool ColorQuantity<QuantityT>::colorsStoredInMemory() {
  return !colors.empty();
}

template <typename QuantityT>
size_t ColorQuantity<QuantityT>::nColorSize() {
  if (colorsStoredInMemory()) {
    return colors.size();
  } else {
    if (!colorRenderBuffer|| !colorRenderBuffer->isSet()) {
      throw std::runtime_error("buffer is not allocated when it should be");
    }
    return static_cast<size_t>(colorRenderBuffer->getDataSize());
  }
}

template <typename QuantityT>
glm::vec3 ColorQuantity<QuantityT>::getColorValue(size_t ind) {
  if (colorsStoredInMemory()) {
    return colors[ind];
  } else {
    return colorRenderBuffer->getData_vec3(ind);
  }
}

template <typename QuantityT>
void ColorQuantity<QuantityT>::renderBufferDataExternallyUpdated() {
  colors.clear();
  requestRedraw();
}

} // namespace polyscope
