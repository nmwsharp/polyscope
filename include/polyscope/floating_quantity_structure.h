// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/polyscope.h"

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/internal.h"
#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/scaled_value.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/color_image_quantity.h"
#include "polyscope/floating_quantity.h"
#include "polyscope/scalar_image_quantity.h"

#include <vector>

namespace polyscope {

// Forward declare the structure
class FloatingQuantityStructure;

// Forward declare quantity types
class Quantity;
class ScalarImageQuantity;
class ColorImageQuantity;


class FloatingQuantityStructure : public QuantityStructure<FloatingQuantityStructure> {
public:
  // === Member functions ===

  // Construct a new structure
  FloatingQuantityStructure(std::string name);
  ~FloatingQuantityStructure();

  // === Overrides

  // Build the imgui display
  virtual void buildUI() override;
  virtual void buildCustomUI() override;
  virtual void buildPickUI(size_t localPickID) override;
  virtual void buildCustomOptionsUI() override;

  // Standard structure overrides
  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void drawPick() override;
  virtual bool hasExtents() override;
  virtual void updateObjectSpaceBounds() override;
  virtual std::string typeName() override;

  // Misc data
  static const std::string structureTypeName;
};

// There is a single, globally shared floating quantity structure to which all floating quantities get registered.
// The pointer lives in internal.h/cpp
FloatingQuantityStructure* getGlobalFloatingQuantityStructure(); // creates it if it doesn't exit yet
void removeFloatingQuantityStructureIfEmpty();

// globals to manage all quantities
void removeFloatingQuantity(std::string name, bool errorIfAbsent = false);
void removeAllFloatingQuantities();

// globals to add/remove particular quantities
template <class T>
ScalarImageQuantity* addScalarImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values,
                                            ImageOrigin imageOrigin, DataType type = DataType::STANDARD);

template <class T>
ColorImageQuantity* addColorImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values_rgb,
                                          ImageOrigin imageOrigin);

template <class T>
ColorImageQuantity* addColorAlphaImageQuantity(std::string name, size_t dimX, size_t dimY, const T& values_rgba,
                                               ImageOrigin imageOrigin);


template <class T1, class T2>
DepthRenderImageQuantity* addDepthRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                      const T2& normalData, ImageOrigin imageOrigin);

template <class T1, class T2, class T3>
ColorRenderImageQuantity* addColorRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                      const T2& normalData, const T3& colorData,
                                                      ImageOrigin imageOrigin);


template <class T1, class T2, class T3>
ScalarRenderImageQuantity* addScalarRenderImageQuantity(std::string name, size_t dimX, size_t dimY, const T1& depthData,
                                                        const T2& normalData, const T3& scalarData,
                                                        ImageOrigin imageOrigin, DataType type = DataType::STANDARD);


} // namespace polyscope

#include "polyscope/floating_quantity_structure.ipp"
