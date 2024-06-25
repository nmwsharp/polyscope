namespace polyscope {

template <typename QuantityT>
TetracolorQuantity<QuantityT>::TetracolorQuantity(QuantityT& quantity_, const std::vector<glm::vec4>& tetracolors_)
  : quantity(quantity_), tetracolors(&quantity, quantity.uniquePrefix() + "tetracolors", tetracolorsData), tetracolorsData(tetracolors_) {} 

} // namespace polyscope
