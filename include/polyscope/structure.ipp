// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/polyscope.h"

#include "polyscope/quantity.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/floating_quantity.h"

namespace polyscope {

// === Floating Quantities ===

template <class T>
ScalarImageQuantity* Structure::addScalarImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values,
                                                       ImageOrigin imageOrigin, DataType type) {
  validateSize(values, dimX * dimY, "floating scalar image " + name);
  return this->addScalarImageQuantityImpl(name, dimX, dimY, standardizeArray<float, T>(values), imageOrigin, type);
}

template <class T>
ColorImageQuantity* Structure::addColorImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values_rgb,
                                                     ImageOrigin imageOrigin) {
  validateSize(values_rgb, dimX * dimY, "floating color image " + name);

  // standardize and pad out the alpha component
  std::vector<glm::vec4> standardVals(standardizeVectorArray<glm::vec4, 3>(values_rgb));
  for (auto& v : standardVals) {
    v.a = 1.;
  }

  return this->addColorImageQuantityImpl(name, dimX, dimY, standardVals, imageOrigin);
}


template <class T>
ColorImageQuantity* Structure::addColorAlphaImageQuantity(std::string name, size_t dimX, size_t dimY,
                                                          const T& values_rgba, ImageOrigin imageOrigin) {
  validateSize(values_rgba, dimX * dimY, "floating color alpha image " + name);

  // standardize
  std::vector<glm::vec4> standardVals(standardizeVectorArray<glm::vec4, 4>(values_rgba));

  return this->addColorImageQuantityImpl(name, dimX, dimY, standardVals, imageOrigin);
}


template <class T1, class T2>
DepthRenderImageQuantity* Structure::addDepthRenderImageQuantity(std::string name, size_t dimX, size_t dimY,
                                                                 const T1& depthData, const T2& normalData,
                                                                 ImageOrigin imageOrigin) {

  validateSize(depthData, dimX * dimY, "depth render image depth data " + name);
  validateSize(normalData, {dimX * dimY, 0}, "depth render image normal data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec3> standardNormal(standardizeVectorArray<glm::vec3, 3>(normalData));

  return this->addDepthRenderImageQuantityImpl(name, dimX, dimY, standardDepth, standardNormal, imageOrigin);
}


template <class T1, class T2, class T3>
ColorRenderImageQuantity* Structure::addColorRenderImageQuantity(std::string name, size_t dimX, size_t dimY,
                                                                 const T1& depthData, const T2& normalData,
                                                                 const T3& colorData, ImageOrigin imageOrigin) {

  validateSize(depthData, dimX * dimY, "depth render image depth data " + name);
  validateSize(normalData, {dimX * dimY, 0}, "depth render image normal data " + name);
  validateSize(colorData, dimX * dimY, "depth render image color data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec3> standardNormal(standardizeVectorArray<glm::vec3, 3>(normalData));
  std::vector<glm::vec3> standardColor(standardizeVectorArray<glm::vec3, 3>(colorData));

  return this->addColorRenderImageQuantityImpl(name, dimX, dimY, standardDepth, standardNormal, standardColor,
                                               imageOrigin);
}


template <class T1, class T2, class T3>
ScalarRenderImageQuantity* Structure::addScalarRenderImageQuantity(std::string name, size_t dimX, size_t dimY,
                                                                   const T1& depthData, const T2& normalData,
                                                                   const T3& scalarData, ImageOrigin imageOrigin,
                                                                   DataType type) {

  validateSize(depthData, dimX * dimY, "depth render image depth data " + name);
  validateSize(normalData, {dimX * dimY, 0}, "depth render image normal data " + name);
  validateSize(scalarData, dimX * dimY, "depth render image scalar data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec3> standardNormal(standardizeVectorArray<glm::vec3, 3>(normalData));
  std::vector<float> standardScalar(standardizeArray<float>(scalarData));

  return this->addScalarRenderImageQuantityImpl(name, dimX, dimY, standardDepth, standardNormal, standardScalar,
                                                imageOrigin, type);
}


template <class T1, class T2>
RawColorRenderImageQuantity* Structure::addRawColorRenderImageQuantity(std::string name, size_t dimX, size_t dimY,
                                                                       const T1& depthData, const T2& colorData,
                                                                       ImageOrigin imageOrigin) {

  validateSize(depthData, dimX * dimY, "depth render image depth data " + name);
  validateSize(colorData, dimX * dimY, "depth render image color data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec3> standardColor(standardizeVectorArray<glm::vec3, 3>(colorData));

  return this->addRawColorRenderImageQuantityImpl(name, dimX, dimY, standardDepth, standardColor, imageOrigin);
}


template <class T1, class T2>
RawColorAlphaRenderImageQuantity*
Structure::addRawColorAlphaRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                               const T2& colorData, ImageOrigin imageOrigin) {

  validateSize(depthData, dimX * dimY, "depth render image depth data " + name);
  validateSize(colorData, dimX * dimY, "depth render image color data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec4> standardColor(standardizeVectorArray<glm::vec4, 4>(colorData));

  return this->addRawColorAlphaRenderImageQuantityImpl(name, dimX, dimY, standardDepth, standardColor, imageOrigin);
}


} // namespace polyscope
