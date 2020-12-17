// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/persistent_value.h"

#include <string>

namespace polyscope {

// A 'quantity' (in Polyscope terminology) is data which is associated with a structure; any structure might have many
// quantities. For instance a mesh structure might have a scalar quantity associated with it, or a point cloud might
// have a vector field quantity associated with it.

template <typename S>
class Quantity {
public:
  Quantity(std::string name, S& parentStructure, bool dominates = false);
  virtual ~Quantity();

  // Draw the quantity.
  virtual void draw();

  // Draw the ImGUI ui elements
  virtual void buildUI();       // draws the tree node and enabled checkbox common to almost all quantities, and calls
                                // drawCustomUI() below. Can still be overidden in case something else is wanted.
  virtual void buildCustomUI(); // overridden by children to add custom data to UI
  virtual void buildPickUI(size_t localPickInd); // overridden by children to add custom fields to pick menu

  // Enable and disable the quantity
  bool isEnabled();
  virtual Quantity<S>* setEnabled(bool newEnabled);

  // = Utility

  // Re-perform any setup work for the quantity, including regenerating shader programs.
  virtual void refresh();

  // A decorated name for the quantity that will be used in headers. For instance, for surface scalar named "value" we
  // return "value (scalar)"
  virtual std::string niceName();
  std::string uniquePrefix();

  // === Member variables ===
  S& parent;              // the parent structure with which this quantity is associated
  const std::string name; // a name for this quantity, which must be unique amongst quantities on `parent`

  // Is this quantity currently being displayed?
  PersistentValue<bool> enabled; // should be set by setEnabled()
  bool dominates = false;
};


} // namespace polyscope


#include "polyscope/quantity.ipp"
