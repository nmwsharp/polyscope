// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

namespace polyscope {

template <typename QuantityT>
TextureMapQuantity<QuantityT>::TextureMapQuantity(QuantityT& quantity_, size_t dimX_, size_t dimY_, ImageOrigin origin_)
    : quantity(quantity_), dimX(dimX_), dimY(dimY_), imageOrigin(origin_),
      filterMode(quantity.uniquePrefix() + "filterMode", FilterMode::Linear) {}

template <typename QuantityT>
void TextureMapQuantity<QuantityT>::buildTextureMapOptionsUI() {

  if (ImGui::BeginMenu("Filter Mode")) {
    if (ImGui::MenuItem("linear", NULL, filterMode.get() == FilterMode::Linear)) setFilterMode(FilterMode::Linear);
    if (ImGui::MenuItem("nearest", NULL, filterMode.get() == FilterMode::Nearest)) setFilterMode(FilterMode::Nearest);
    ImGui::EndMenu();
  }
}

template <typename QuantityT>
QuantityT* TextureMapQuantity<QuantityT>::setFilterMode(FilterMode newFilterMode) {
  filterMode = newFilterMode;
  quantity.refresh();
  return &quantity;
}

template <typename QuantityT>
FilterMode TextureMapQuantity<QuantityT>::getFilterMode() {
  return filterMode.get();
}

} // namespace polyscope