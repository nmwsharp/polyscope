// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

namespace polyscope {

template <typename QuantityT>
ColorQuantity<QuantityT>::ColorQuantity(QuantityT& quantity_, const std::vector<glm::vec3>& colors_)
    : quantity(quantity_), colors(quantity.uniquePrefix() + "#colors", colorsData), colorsData(colors_) {}

template <typename QuantityT>
void ColorQuantity<QuantityT>::buildColorUI() {}

template <typename QuantityT>
void ColorQuantity<QuantityT>::setColorUniforms(render::ShaderProgram& p) {}

template <typename QuantityT>
std::vector<std::string> ColorQuantity<QuantityT>::addColorRules(std::vector<std::string> rules) {
  return rules;
}


template <typename QuantityT>
template <class V>
void ColorQuantity<QuantityT>::updateData(const V& newColors) {
  validateSize(newColors, colors.size(), "color quantity");
  colors.data = standardizeVectorArray<glm::vec3, 3>(newColors);
  colors.markHostBufferUpdated();
}


} // namespace polyscope
