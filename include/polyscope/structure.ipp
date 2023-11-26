// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/polyscope.h"

#include "polyscope/quantity.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/floating_quantity.h"

namespace polyscope {

// === Derived structure can manage quantities

template <typename S>
QuantityStructure<S>::QuantityStructure(std::string name_, std::string subtypeName) : Structure(name_, subtypeName) {}

template <typename S>
QuantityStructure<S>::~QuantityStructure(){};

template <typename S>
void QuantityStructure<S>::checkForQuantityWithNameAndDeleteOrError(std::string name, bool allowReplacement) {

  // Look for an existing quantity with this name
  bool quantityExists = quantities.find(name) != quantities.end();
  bool floatingQuantityExists = floatingQuantities.find(name) != floatingQuantities.end();

  // if it already exists and we cannot replace, throw an error
  if (!allowReplacement && (quantityExists || floatingQuantityExists)) {
    exception("Tried to add quantity with name: [" + name +
              "], but a quantity with that name already exists on the structure [" + name +
              "]. Use the allowReplacement option like addQuantity(..., true) to replace.");
  }

  // Remove the old quantity
  if (quantityExists || floatingQuantityExists) {
    removeQuantity(name);
  }
}

template <typename S>
void QuantityStructure<S>::addQuantity(QuantityType* q, bool allowReplacement) {

  // Check if a quantity with this name exists, remove it or throw and error if so
  checkForQuantityWithNameAndDeleteOrError(q->name, allowReplacement);

  // Add the new quantity
  quantities[q->name] = std::unique_ptr<QuantityType>(q);
}

template <typename S>
void QuantityStructure<S>::addQuantity(FloatingQuantity* q, bool allowReplacement) {

  // Check if a quantity with this name exists, remove it or throw and error if so
  checkForQuantityWithNameAndDeleteOrError(q->name, allowReplacement);

  // Add the new quantity
  floatingQuantities[q->name] = std::unique_ptr<FloatingQuantity>(q);
}

template <typename S>
typename QuantityStructure<S>::QuantityType* QuantityStructure<S>::getQuantity(std::string name) {
  if (quantities.find(name) == quantities.end()) {
    return nullptr;
  }
  return quantities[name].get();
}

template <typename S>
FloatingQuantity* QuantityStructure<S>::getFloatingQuantity(std::string name) {
  if (floatingQuantities.find(name) == floatingQuantities.end()) {
    return nullptr;
  }
  return floatingQuantities[name].get();
}

template <typename S>
void QuantityStructure<S>::refresh() {
  for (auto& qp : quantities) {
    qp.second->refresh();
  }
  for (auto& qp : floatingQuantities) {
    qp.second->refresh();
  }
  requestRedraw();
}

template <typename S>
void QuantityStructure<S>::removeQuantity(std::string name, bool errorIfAbsent) {

  // Look for an existing quantity with this name
  bool quantityExists = quantities.find(name) != quantities.end();
  bool floatingQuantityExists = floatingQuantities.find(name) != floatingQuantities.end();

  if (errorIfAbsent && !(quantityExists || floatingQuantityExists)) {
    exception("No quantity named " + name + " added to structure " + name);
    return;
  }

  // delete standard quantities
  if (quantityExists) {
    // If this is the active quantity, clear it
    QuantityType& q = *quantities[name];
    if (dominantQuantity == &q) {
      clearDominantQuantity();
    }

    // Delete the quantity
    quantities.erase(name);
  }

  // delete floating quantities
  if (floatingQuantityExists) {
    floatingQuantities.erase(name);
  }
}

template <typename S>
void QuantityStructure<S>::removeAllQuantities() {
  while (quantities.size() > 0) {
    removeQuantity(quantities.begin()->first);
  }
  while (floatingQuantities.size() > 0) {
    removeQuantity(floatingQuantities.begin()->first);
  }
}

template <typename S>
void QuantityStructure<S>::setDominantQuantity(QuantityS<S>* q) {
  if (!q->dominates) {
    exception("tried to set dominant quantity with quantity that has dominates=false");
    return;
  }

  // Dominant quantity must be enabled
  q->setEnabled(true);

  // All other dominating quantities will be disabled
  for (auto& qp : quantities) {
    QuantityType* qOther = qp.second.get();
    if (qOther->dominates && qOther->isEnabled() && qOther != q) {
      qOther->setEnabled(false);
    }
  }

  dominantQuantity = q;
}

template <typename S>
void QuantityStructure<S>::clearDominantQuantity() {
  dominantQuantity = nullptr;
}

template <typename S>
void QuantityStructure<S>::setAllQuantitiesEnabled(bool newEnabled) {
  for (auto& x : quantities) {
    x.second->setEnabled(newEnabled);
  }
  for (auto& x : floatingQuantities) {
    x.second->setEnabled(newEnabled);
  }
}


template <typename S>
void QuantityStructure<S>::buildQuantitiesUI() {
  // Build the quantities
  for (auto& x : quantities) {
    x.second->buildUI();
  }
  for (auto& x : floatingQuantities) {
    x.second->buildUI();
  }
}

template <typename S>
void QuantityStructure<S>::buildStructureOptionsUI() {
  if (ImGui::BeginMenu("Quantity Selection")) {
    if (ImGui::MenuItem("Enable all")) setAllQuantitiesEnabled(true);
    if (ImGui::MenuItem("Disable all")) setAllQuantitiesEnabled(false);
    ImGui::EndMenu();
  }
}

// === Floating Quantities ===

template <typename S>
template <class T>
ScalarImageQuantity* QuantityStructure<S>::addScalarImageQuantity(std::string name, size_t dimX, size_t dimY,
                                                                  const T& values, ImageOrigin imageOrigin,
                                                                  DataType type) {
  validateSize(values, dimX * dimY, "floating scalar image " + name);
  return this->addScalarImageQuantityImpl(name, dimX, dimY, standardizeArray<float, T>(values), imageOrigin, type);
}


template <typename S>
template <class T>
ColorImageQuantity* QuantityStructure<S>::addColorImageQuantity(std::string name, size_t dimX, size_t dimY,
                                                                const T& values_rgb, ImageOrigin imageOrigin) {
  validateSize(values_rgb, dimX * dimY, "floating color image " + name);

  // standardize and pad out the alpha component
  std::vector<glm::vec4> standardVals(standardizeVectorArray<glm::vec4, 3>(values_rgb));
  for (auto& v : standardVals) {
    v.a = 1.;
  }

  return this->addColorImageQuantityImpl(name, dimX, dimY, standardVals, imageOrigin);
}


template <typename S>
template <class T>
ColorImageQuantity* QuantityStructure<S>::addColorAlphaImageQuantity(std::string name, size_t dimX, size_t dimY,
                                                                     const T& values_rgba, ImageOrigin imageOrigin) {
  validateSize(values_rgba, dimX * dimY, "floating color alpha image " + name);

  // standardize
  std::vector<glm::vec4> standardVals(standardizeVectorArray<glm::vec4, 4>(values_rgba));

  return this->addColorImageQuantityImpl(name, dimX, dimY, standardVals, imageOrigin);
}

template <typename S>
template <class T1, class T2>
DepthRenderImageQuantity* QuantityStructure<S>::addDepthRenderImageQuantity(std::string name, size_t dimX, size_t dimY,
                                                                            const T1& depthData, const T2& normalData,
                                                                            ImageOrigin imageOrigin) {

  validateSize(depthData, dimX * dimY, "depth render image depth data " + name);
  validateSize(normalData, {dimX * dimY, 0}, "depth render image normal data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec3> standardNormal(standardizeVectorArray<glm::vec3, 3>(normalData));

  return this->addDepthRenderImageQuantityImpl(name, dimX, dimY, standardDepth, standardNormal, imageOrigin);
}

template <typename S>
template <class T1, class T2, class T3>
ColorRenderImageQuantity*
QuantityStructure<S>::addColorRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                  const T2& normalData, const T3& colorData, ImageOrigin imageOrigin) {

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

template <typename S>
template <class T1, class T2, class T3>
ScalarRenderImageQuantity*
QuantityStructure<S>::addScalarRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                   const T2& normalData, const T3& scalarData, ImageOrigin imageOrigin,
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

template <typename S>
template <class T1, class T2>
RawColorRenderImageQuantity*
QuantityStructure<S>::addRawColorRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                     const T2& colorData, ImageOrigin imageOrigin) {

  validateSize(depthData, dimX * dimY, "depth render image depth data " + name);
  validateSize(colorData, dimX * dimY, "depth render image color data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec3> standardColor(standardizeVectorArray<glm::vec3, 3>(colorData));

  return this->addRawColorRenderImageQuantityImpl(name, dimX, dimY, standardDepth, standardColor, imageOrigin);
}

template <typename S>
template <class T1, class T2>
RawColorAlphaRenderImageQuantity* QuantityStructure<S>::addRawColorAlphaRenderImageQuantity(
    std::string name, size_t dimX, size_t dimY, const T1& depthData, const T2& colorData, ImageOrigin imageOrigin) {

  validateSize(depthData, dimX * dimY, "depth render image depth data " + name);
  validateSize(colorData, dimX * dimY, "depth render image color data " + name);

  // standardize
  std::vector<float> standardDepth(standardizeArray<float>(depthData));
  std::vector<glm::vec4> standardColor(standardizeVectorArray<glm::vec4, 4>(colorData));

  return this->addRawColorAlphaRenderImageQuantityImpl(name, dimX, dimY, standardDepth, standardColor, imageOrigin);
}

// === Floating Quantity Impls ===

// Forward declare helper functions, which wrap the constructors for the floating quantities below.
// Otherwise, we would have to include their respective headers here, and create some really gnarly header dependency
// chains.
ScalarImageQuantity* createScalarImageQuantity(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                               const std::vector<float>& data, ImageOrigin imageOrigin,
                                               DataType dataType);
ColorImageQuantity* createColorImageQuantity(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                             const std::vector<glm::vec4>& data, ImageOrigin imageOrigin);
DepthRenderImageQuantity* createDepthRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                 const std::vector<float>& depthData,
                                                 const std::vector<glm::vec3>& normalData, ImageOrigin imageOrigin);

ColorRenderImageQuantity* createColorRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                 const std::vector<float>& depthData,
                                                 const std::vector<glm::vec3>& normalData,
                                                 const std::vector<glm::vec3>& colorData, ImageOrigin imageOrigin);


RawColorRenderImageQuantity* createRawColorRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                       const std::vector<float>& depthData,
                                                       const std::vector<glm::vec3>& colorData,
                                                       ImageOrigin imageOrigin);

RawColorAlphaRenderImageQuantity* createRawColorAlphaRenderImage(Structure& parent, std::string name, size_t dimX,
                                                                 size_t dimY, const std::vector<float>& depthData,
                                                                 const std::vector<glm::vec4>& colorData,
                                                                 ImageOrigin imageOrigin);


ScalarRenderImageQuantity* createScalarRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                   const std::vector<float>& depthData,
                                                   const std::vector<glm::vec3>& normalData,
                                                   const std::vector<float>& scalarData, ImageOrigin imageOrigin,
                                                   DataType type);

template <typename S>
ScalarImageQuantity* QuantityStructure<S>::addScalarImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                                      const std::vector<float>& values,
                                                                      ImageOrigin imageOrigin, DataType type) {
  checkForQuantityWithNameAndDeleteOrError(name);
  ScalarImageQuantity* q = createScalarImageQuantity(*this, name, dimX, dimY, values, imageOrigin, type);
  addQuantity(q);
  return q;
}

template <typename S>
ColorImageQuantity* QuantityStructure<S>::addColorImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                                    const std::vector<glm::vec4>& values,
                                                                    ImageOrigin imageOrigin) {
  checkForQuantityWithNameAndDeleteOrError(name);
  ColorImageQuantity* q = createColorImageQuantity(*this, name, dimX, dimY, values, imageOrigin);
  addQuantity(q);
  return q;
}

template <typename S>
DepthRenderImageQuantity* QuantityStructure<S>::addDepthRenderImageQuantityImpl(
    std::string name, size_t dimX, size_t dimY, const std::vector<float>& depthData,
    const std::vector<glm::vec3>& normalData, ImageOrigin imageOrigin) {
  checkForQuantityWithNameAndDeleteOrError(name);
  DepthRenderImageQuantity* q = createDepthRenderImage(*this, name, dimX, dimY, depthData, normalData, imageOrigin);
  addQuantity(q);
  return q;
}

template <typename S>
ColorRenderImageQuantity* QuantityStructure<S>::addColorRenderImageQuantityImpl(
    std::string name, size_t dimX, size_t dimY, const std::vector<float>& depthData,
    const std::vector<glm::vec3>& normalData, const std::vector<glm::vec3>& colorData, ImageOrigin imageOrigin) {
  checkForQuantityWithNameAndDeleteOrError(name);
  ColorRenderImageQuantity* q =
      createColorRenderImage(*this, name, dimX, dimY, depthData, normalData, colorData, imageOrigin);
  addQuantity(q);
  return q;
}

template <typename S>
ScalarRenderImageQuantity* QuantityStructure<S>::addScalarRenderImageQuantityImpl(
    std::string name, size_t dimX, size_t dimY, const std::vector<float>& depthData,
    const std::vector<glm::vec3>& normalData, const std::vector<float>& scalarData, ImageOrigin imageOrigin,
    DataType type) {
  checkForQuantityWithNameAndDeleteOrError(name);
  ScalarRenderImageQuantity* q =
      createScalarRenderImage(*this, name, dimX, dimY, depthData, normalData, scalarData, imageOrigin, type);
  addQuantity(q);
  return q;
}

template <typename S>
RawColorRenderImageQuantity* QuantityStructure<S>::addRawColorRenderImageQuantityImpl(
    std::string name, size_t dimX, size_t dimY, const std::vector<float>& depthData,
    const std::vector<glm::vec3>& colorData, ImageOrigin imageOrigin) {
  checkForQuantityWithNameAndDeleteOrError(name);
  RawColorRenderImageQuantity* q =
      createRawColorRenderImage(*this, name, dimX, dimY, depthData, colorData, imageOrigin);
  addQuantity(q);
  return q;
}

template <typename S>
RawColorAlphaRenderImageQuantity* QuantityStructure<S>::addRawColorAlphaRenderImageQuantityImpl(
    std::string name, size_t dimX, size_t dimY, const std::vector<float>& depthData,
    const std::vector<glm::vec4>& colorData, ImageOrigin imageOrigin) {
  checkForQuantityWithNameAndDeleteOrError(name);
  RawColorAlphaRenderImageQuantity* q =
      createRawColorAlphaRenderImage(*this, name, dimX, dimY, depthData, colorData, imageOrigin);
  addQuantity(q);
  return q;
}

} // namespace polyscope
