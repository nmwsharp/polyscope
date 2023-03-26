// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

namespace polyscope {


// globals are implemented by just forwarding to the singleton structure

template <class T>
ScalarImageQuantity* addScalarImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values,
                                            ImageOrigin imageOrigin, DataType type) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addScalarImageQuantity(name, dimX, dimY, values, imageOrigin, type);
}

template <class T>
ColorImageQuantity* addColorImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values_rgb,
                                          ImageOrigin imageOrigin) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addColorImageQuantity(name, dimX, dimY, values_rgb, imageOrigin);
}

template <class T>
ColorImageQuantity* addColorAlphaImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values_rgba,
                                               ImageOrigin imageOrigin) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addColorImageQuantity(name, dimX, dimY, values_rgba, imageOrigin);
}


template <class T1, class T2>
DepthRenderImageQuantity* addDepthRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                      const T2& normalData, ImageOrigin imageOrigin) {

  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addDepthRenderImageQuantity(name, dimX, dimY, depthData, normalData, imageOrigin);
}

template <class T1, class T2, class T3>
ColorRenderImageQuantity* addColorRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                      const T2& normalData, const T3& colorData,
                                                      ImageOrigin imageOrigin) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addColorRenderImageQuantity(name, dimX, dimY, depthData, normalData, colorData, imageOrigin);
}


template <class T1, class T2, class T3>
ScalarRenderImageQuantity* addScalarRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                        const T2& normalData, const T3& scalarData,
                                                        ImageOrigin imageOrigin, DataType type) {
  FloatingQuantityStructure* q = getGlobalFloatingQuantityStructure();
  return q->addScalarRenderImageQuantity(name, dimX, dimY, depthData, normalData, scalarData, imageOrigin, type);
}


} // namespace polyscope
