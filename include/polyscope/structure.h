#pragma once

#include <map>
#include <string>
#include <iostream>

#include "geometrycentral/vector3.h"

namespace polyscope {


// A 'structure' in Polyscope terms, is an object with which we can associate data in the UI, such as a point cloud,
// or a mesh. This in contrast to 'quantities', which we associate with the structures. For instance, a surface mesh
// (which is a structure), might have two scalar functions and a vector field associated with it (which are quantities).
//
// Polyscope expects that (1) the name member is unique of all structures with that type registered with the UI, and (2)
// the type member is a consistent name for the type (I strongly suggest setting it from a constant in the derived
// type). These two properties are not great polymorphic design, but they seem to be the lowest-effort way to allow a
// user to utilize and access custom structures with little code.

class Structure {
 public:
  // === Member functions ===

  // Base constructor which sets the name
  Structure(std::string name, std::string type);

  virtual ~Structure() = 0;

  // Render the the structure on screen
  virtual void draw() = 0;

  // = Do setup work related to drawing, including allocating openGL data
  virtual void prepare() = 0;
  virtual void preparePick() = 0;

  // = Build the imgui display
  // Draw the actual UI
  virtual void drawUI() = 0;
  // Draw any UI elements 
  virtual void drawSharedStructureUI() = 0;
  // Draw pick UI elements when index localPickID is selected
  virtual void drawPickUI(size_t localPickID) = 0;

  // Render to pick buffer
  virtual void drawPick() = 0;

  // A characteristic length for the structure
  virtual double lengthScale() = 0;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
  boundingBox() = 0;

  // === Member variables ===
  const std::string name; // should be unique amongst registered structures with this type
  const std::string type; // must be a consistent name for all instances of a derived subclass 
};


}  // namespace polyscope
