#pragma once

namespace polyscope {


// globals are implemented by just forwarding to the singleton structure

template <class T>
FloatingScalarImageQuantity* addFloatingScalarImage(std::string name, size_t dimX, size_t dimY, const T& values,
                                                    DataType type) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addFloatingScalarImage(name, dimX, dimY, values, type);
}

template <class T>
FloatingColorImageQuantity* addFloatingColorImage(std::string name, size_t dimX, size_t dimY, const T& values_rgb) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addFloatingColorImage(name, dimX, dimY, values_rgb);
}

template <class T>
FloatingColorImageQuantity* addFloatingColorAlphaImage(std::string name, size_t dimX, size_t dimY,
                                                       const T& values_rgba) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addFloatingColorImage(name, dimX, dimY, values_rgba);
}


} // namespace polyscope
