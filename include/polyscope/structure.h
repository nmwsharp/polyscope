#pragma once

#include <iostream>
#include <map>
#include <string>

#include "glm/glm.hpp"

namespace polyscope {


// A 'structure' in Polyscope terms, is an object with which we can associate data in the UI, such as a point cloud,
// or a mesh. This in contrast to 'quantities', which we associate with the structures. For instance, a surface mesh
// (which is a structure), might have two scalar functions and a vector field associated with it (which are quantities).
//
// Polyscope expects that (1) the name member is unique of all structures with that type registered with the UI, and (2)
// the type member is a consistent name for the type (I strongly suggest setting it from a constant in the derived
// type). These two properties are not great polymorphic design, but they seem to be the lowest-effort way to allow a
// user to utilize and access custom structures with little code.

template <typename S> // template on the derived type
class Structure {
public:
  // === Member functions ===

  // Base constructor which sets the name
  Structure(std::string name, std::string type);

  virtual ~Structure() = 0;

  // Render the the structure on screen
  virtual void draw() = 0;

  // = Build the imgui display

  // Draw the ImGUI ui elements
  void drawUI();
  virtual void drawCustomUI() = 0; // overridden by childen to add custom UI data

  // Draw any UI elements
  virtual void drawSharedStructureUI() = 0;

  // Draw pick UI elements when index localPickID is selected
  virtual void drawPickUI(size_t localPickID) = 0;

  // Render to pick buffer
  virtual void drawPick() = 0;

  // A characteristic length for the structure
  virtual double lengthScale() = 0;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<glm::vec3, glm::vec3> boundingBox() = 0;

  // Identifying data
  const std::string name;             // should be unique amongst registered structures with this type
  virtual std::string typeName() = 0; // a name which identifies the type, like "point cloud"

  // Scene transform
  glm::mat4 objectTransform = glm::mat4(1.0);
  void centerBoundingBox();
  void resetTransform();
  glm::mat4 getModelView();

  // Manage quantities
  setDominantQuantity(Quantity<S>* q);
  void clearDominantQuantity();

protected:
  // Quantities
  std::map<std::string, std::unique_ptr<Quantity<S>>> quantities;
  Quantity<S>* dominantQuantity = nullptr; // if non-null, a special quantity of which only one can be drawn for
                                           // the structure. handles common case of a surface color, e.g. color of
                                           // a mesh or point cloud the dominant quantity must always be enabled
};


} // namespace polyscope
