// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/curve_network_quantity.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/polyscope.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/curve_network_color_quantity.h"
#include "polyscope/curve_network_scalar_quantity.h"
#include "polyscope/curve_network_vector_quantity.h"

#include <vector>

#include "marchingcubes/mesh_implicit_surface.h"

namespace polyscope {


class ImplicitSurface : public QuantityStructure<ImplicitSurface> {
public:
  // Construct a new curve network structure
  ImplicitSurface(std::string name, double* field, size_t nValuesPerSide, glm::vec3 center, double sideLen);

  // === Overloads

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildPickUI(size_t localPickID) override;
  // Render the the structure on screen
  virtual void draw() override;
  // Render for picking
  virtual void drawPick() override;
  // A characteristic length for the structure
  virtual double lengthScale() override;
  // Axis-aligned bounding box for the structure
  virtual std::tuple<glm::vec3, glm::vec3> boundingBox() override;
  virtual std::string typeName() override;

  // Field data
  double* field;
  size_t nCornersPerSide;

  inline size_t nCells() const { return nCornersPerSide * nCornersPerSide * nCornersPerSide; }

  // Misc data
  static const std::string structureTypeName;

private:
  glm::vec3 gridCenter;
  double sideLength;
  PersistentValue<glm::vec3> color;
  ImplicitSurface* setColor(glm::vec3 newVal);
  glm::vec3 getColor();

};
} // namespace polyscope