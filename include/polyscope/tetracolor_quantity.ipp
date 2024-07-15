namespace polyscope {

template <typename QuantityT>
TetracolorQuantity<QuantityT>::TetracolorQuantity(QuantityT& quantity_, const std::vector<glm::vec4>& tetracolors_)
  : quantity(quantity_), tetracolors(&quantity, quantity.uniquePrefix() + "tetracolors", tetracolorsData), tetracolorsData(tetracolors_) {} 

template <typename QuantityT>
void TetracolorQuantity<QuantityT>::setTetracolorUniforms(render::ShaderProgram& p) {}

template <typename QuantityT>
std::vector<std::string> TetracolorQuantity<QuantityT>::addTetracolorRules(std::vector<std::string> rules) {
  return rules;
}

} // namespace polyscope
