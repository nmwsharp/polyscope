// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/scaled_value.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/floating_color_image.h"
#include "polyscope/floating_quantity.h"
#include "polyscope/floating_scalar_image.h"

#include <vector>

namespace polyscope {

// Forward declare the structure
class FloatingQuantityStructure;

// Forward declare quantity types
class FloatingScalarImageQuantity;
class FloatingColorImageQuantity;


template <> // Specialize the quantity type
struct QuantityTypeHelper<FloatingQuantityStructure> {
  typedef FloatingQuantity type;
};

class FloatingQuantityStructure : public QuantityStructure<FloatingQuantityStructure> {
public:
  // === Member functions ===

  // Construct a new structure
  FloatingQuantityStructure(std::string name);

  // === Overrides

  // Build the imgui display
  virtual void buildUI() override;
  virtual void buildCustomUI() override;
  virtual void buildCustomOptionsUI() override;

  // Standard structure overrides
  virtual void draw() override;
  virtual bool hasExtents() override;
  virtual double lengthScale() override;
  virtual std::tuple<glm::vec3, glm::vec3> boundingBox() override;
  virtual std::string typeName() override;

  // === Quantities
  // public here unlke other structures, since we add to the global class from free functions
  FloatingScalarImageQuantity* addFloatingScalarImageImpl(std::string name, size_t dimX, size_t dimY,
                                                          const std::vector<double>& values, DataType type);

  FloatingColorImageQuantity* addFloatingColorImageImpl(std::string name, size_t dimX, size_t dimY,
                                                        const std::vector<glm::vec4>& values);

  // Misc data
  static const std::string structureTypeName;
};

// There is a single, globally shared floating quantity structure to which all floating quantities get registered.
static FloatingQuantityStructure* globalFloatingQuantityStructure = nullptr;
FloatingQuantityStructure* getGlobalFloatingQuantityStructure(); // creates it if it doesn't exit yet
void removeFloatingQuantityStructureIfEmpty();


// add/remove quantities
template <class T>
FloatingScalarImageQuantity* addFloatingScalarImage(std::string name, size_t dimX, size_t dimY, const T& values,
                                                    DataType type = DataType::STANDARD);
void removeFloatingScalarImage(std::string name);

template <class T>
FloatingColorImageQuantity* addFloatingColorImage(std::string name, size_t dimX, size_t dimY, const T& values_rgb);
void removeFloatingColorImage(std::string name);

} // namespace polyscope

#include "polyscope/floating_quantity_structure.ipp"
