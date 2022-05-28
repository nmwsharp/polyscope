#pragma once

namespace polyscope {


// globals are implemented by just forwarding to the singleton structure

template <class T>
ScalarImageQuantity* addScalarImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values,
                                            DataType type) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addScalarImageQuantity(name, dimX, dimY, values, type);
}

template <class T>
ColorImageQuantity* addColorImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values_rgb) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addColorImageQuantity(name, dimX, dimY, values_rgb);
}

template <class T>
ColorImageQuantity* addColorAlphaImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values_rgba) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addColorImageQuantity(name, dimX, dimY, values_rgba);
}


template <class T1, class T2>
DepthRenderImageQuantity* addDepthRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData) {

  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addDepthRenderImageQuantity(name, dimX, dimY, depthData);
}

template <class T1, class T2, class T3>
ColorRenderImageQuantity* addColorRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                      const T2& normalData) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addColorRenderImageQuantity(name, dimX, dimY, depthData, normalData);
}


template <class T1, class T2, class T3>
ScalarRenderImageQuantity* addScalarRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                        const T2& normalData, const T3& scalarData, DataType type) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addScalarRenderImageQuantity(name, dimX, dimY, depthData, normalData, scalarData, type);
}


} // namespace polyscope
