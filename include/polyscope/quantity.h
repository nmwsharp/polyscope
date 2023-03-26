// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/persistent_value.h"

#include <string>

namespace polyscope {

// A 'quantity' (in Polyscope terminology) is data which is associated with a structure; any structure might have many
// quantities. For instance a mesh structure might have a scalar quantity associated with it, or a point cloud might
// have a vector field quantity associated with it.

// forward decalaration
class Structure;

// === General Quantities
// (subclasses could be a structure-specific quantity or a floating quantity)

class Quantity {

public:
  Quantity(std::string name, Structure& parentStructure);
  virtual ~Quantity();

  // Draw the quantity.
  virtual void draw();
  virtual void drawDelayed(); // drawing that should happen after the main phase

  // Draw the ImGUI ui elements
  virtual void buildUI();       // draws the tree node and enabled checkbox common to almost all quantities, and calls
                                // drawCustomUI() below. Can still be overidden in case something else is wanted.
  virtual void buildCustomUI(); // overridden by children to add custom data to UI
  virtual void buildPickUI(size_t localPickInd); // overridden by children to add custom fields to pick menu

  // Enable and disable the quantity
  bool isEnabled();
  // there is no setEnabled() here, only in subclasses, because subclasses have different return types

  // = Utility

  // Re-perform any setup work for the quantity, including regenerating shader programs.
  virtual void refresh();

  // A decorated name for the quantity that will be used in headers. For instance, for surface scalar named "value" we
  // return "value (scalar)"
  virtual std::string niceName();
  std::string uniquePrefix();

  // === Member variables ===
  Structure& parent;      // the parent structure with which this quantity is associated
  const std::string name; // a name for this quantity, which must be unique amongst quantities on `parent`

  // Is this quantity currently being displayed?
  PersistentValue<bool> enabled; // should be set by setEnabled()
};

// === Structure-specific Quantities

template <typename S>
class QuantityS : public Quantity {
public:
  QuantityS(std::string name, S& parentStructure, bool dominates = false);
  virtual ~QuantityS();

  // Enable and disable the quantity
  virtual QuantityS<S>* setEnabled(bool newEnabled);

  virtual void buildUI() override;

  S& parent; // note: this HIDES the more general member of the same name in the parent class

  // Track dominating quantities
  bool dominates = false;
};


} // namespace polyscope


#include "polyscope/quantity.ipp"
