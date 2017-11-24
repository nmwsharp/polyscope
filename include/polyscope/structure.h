#pragma once

#include "geometrycentral/vector3.h"

namespace polyscope {

class Structure {
 public:
  // === Member functions ===

  // Base constructor which sets the name
  Structure(std::string name);

  virtual ~Structure() = 0;

  // Render the the structure on screen
  virtual void draw() = 0;

  // Do setup work related to drawing, including allocating openGL data
  virtual void prepare() = 0;

  // Undo anything done in prepare(), including deallocating openGL data
  virtual void teardown() = 0;

  // Build the imgui display
  virtual void drawUI() = 0;

  // Render for picking
  virtual void drawPick() = 0;

  // A characteristic length for the structure
  virtual double lengthScale() = 0;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
  boundingBox() = 0;

  // === Member variables ===
  const std::string name;
};

}  // namespace polyscope