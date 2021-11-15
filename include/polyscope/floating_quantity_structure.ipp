#pragma once

namespace polyscope {

template <class T>
FloatingScalarImageQuantity* addFloatingScalarImage(std::string name, size_t dimX, size_t dimY, const T& values,
                                                    DataType type) {
  FloatingQuantityStructure* g = getGlobalFloatingQuantityStructure();
  validateSize(values, dimX * dimY, "floating scalar image " + name);
  return g->addFloatingScalarImageImpl(name, dimX, dimY, standardizeArray<double, T>(values), type);
}


template <class T>
FloatingColorImageQuantity* addFloatingColorImage(std::string name, size_t dimX, size_t dimY, const T& values_rgb) {
  FloatingQuantityStructure* g = getGlobalFloatingQuantityStructure();
  validateSize(values_rgb, dimX * dimY, "floating color image " + name);

  // standardize and pad out the alpha component 
  std::vector<glm::vec4> standardVals(standardizeVectorArray<glm::vec4, 3>(values_rgb));
  for (auto& v : standardVals) {
    v.a = 1.;
  }

  return g->addFloatingColorImageImpl(name, dimX, dimY, standardVals);
}

} // namespace polyscope
