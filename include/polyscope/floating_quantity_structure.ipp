#pragma once

namespace polyscope {


// globals are implemented by just forwarding to the singleton structure

template <class T>
ScalarImageQuantity* addScalarImage(std::string name, size_t dimX, size_t dimY, const T& values, DataType type) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addScalarImage(name, dimX, dimY, values, type);
}

template <class T>
ColorImageQuantity* addColorImage(std::string name, size_t dimX, size_t dimY, const T& values_rgb) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addColorImage(name, dimX, dimY, values_rgb);
}

template <class T>
ColorImageQuantity* addColorAlphaImage(std::string name, size_t dimX, size_t dimY, const T& values_rgba) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addColorImage(name, dimX, dimY, values_rgba);
}


} // namespace polyscope
