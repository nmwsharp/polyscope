#pragma once

namespace polyscope {

// A 'quantity' (in Polyscope terminology) is data which is associated with a structure; any structure might have many
// quantities. For instance a mesh structure might have a scalar quantity associated with it, or a point cloud might
// have a vector field quantity associated with it.

template <typename S>
class Quantity {

  Quantity(S& parentStructure, std::string name);
  virtual ~Quantity() = 0;

  // Draw the quantity.
  virtual void draw() = 0;

  // Draw the ImGUI ui elements
  void drawUI();
  virtual void drawCustomUI() = 0; // overridden by children to add custom data to UI

  // Enable and disable the quantity
  bool isEnabled();
  virtual void setEnabled(bool newEnabled);

  // === Member variables ===
  S& parent;              // the parent structure with which this quantity is associated
  const std::string name; // a name for this quantity, which must be unique amongst quantities on `parent`

protected:

  // Is this quantity currently being displayed?
  bool enabled = false; // should be set by setEnabled()
  const bool dominates = false;
};


} // namespace polyscope


#include "polyscope/quantity.ipp"
