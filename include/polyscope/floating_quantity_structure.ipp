#pragma once

namespace polyscope {

template <class T>
FloatingScalarImageQuantity* addFloatingScalarImage(std::string name, size_t dimX, size_t dimY, const T& values,
                                                    DataType type) {
  FloatingQuantityStructure* g = getGlobalFloatingQuantityStructure();
  validateSize(values, dimX * dimY, "floating scalar image " + name);
  return g->addFloatingScalarImageImpl(name, dimX, dimY, standardizeArray<double, T>(values), type);
}

} // namespace polyscope
