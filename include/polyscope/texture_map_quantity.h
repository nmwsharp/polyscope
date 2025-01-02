// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/standardize_data_array.h"

namespace polyscope {

// Encapsulates logic which is common to all texture map quantities

template <typename QuantityT>
class TextureMapQuantity {
public:
  TextureMapQuantity(QuantityT& parent, size_t dimX, size_t dimY, ImageOrigin origin_);

  // Build the ImGUI UIs for texture maps
  virtual void buildTextureMapOptionsUI(); // called inside of an options menu

  // === Members
  QuantityT& quantity;

  // NOTE: the main quantity types (scalar quantity, color quantity, etc) provide the buffer members, so this class just
  // has secondary options and such

  // what kind of texture filtering is used
  QuantityT* setFilterMode(FilterMode newFilterMode);
  FilterMode getFilterMode();

protected:
  size_t dimX, dimY;
  ImageOrigin imageOrigin;

  // === Visualization parameters
  PersistentValue<FilterMode> filterMode; // default is FilterMode::Linear
};

} // namespace polyscope

#include "polyscope/texture_map_quantity.ipp"