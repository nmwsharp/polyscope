// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/polyscope.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/volumetric_grid_quantity.h"

#include <vector>

#include "marchingcubes/mesh_implicit_surface.h"

namespace polyscope {

class VolumetricGrid;

class VolumetricGridScalarIsosurface;

template <> // Specialize the quantity type
struct QuantityTypeHelper<VolumetricGrid> {
  typedef VolumetricGridQuantity type;
};


class VolumetricGrid : public QuantityStructure<VolumetricGrid> {
public:
  // Construct a new curve network structure
  VolumetricGrid(std::string name, const std::vector<double> &field, size_t nValuesPerSide, glm::vec3 center, double sideLen);

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
  std::vector<double> field;
  size_t nCornersPerSide;
  glm::vec3 gridCenter;
  double sideLength;
  double levelSet;

  inline size_t nValues() const { return nCornersPerSide * nCornersPerSide * nCornersPerSide; }

  void meshCurrentLevelSet();

  // Misc data
  static const std::string structureTypeName;

  template <class T>
  VolumetricGridScalarIsosurface* addGridIsosurfaceQuantity(std::string name, const T& values);

private:
  PersistentValue<glm::vec3> color;
  VolumetricGrid* setColor(glm::vec3 newVal);
  glm::vec3 getColor();
  VolumetricGridScalarIsosurface* addIsosurfaceQuantityImpl(std::string name, const std::vector<double>& data);
};



template <class T>
VolumetricGridScalarIsosurface* VolumetricGrid::addGridIsosurfaceQuantity(std::string name, const T& values) {
  validateSize(values, nValues(), "grid isosurface quantity " + name);
  return addIsosurfaceQuantityImpl(name, standardizeArray<double, T>(values));
}

template<typename T>
VolumetricGrid* registerImplicitSurfaceGrid(std::string name, const T &field,
size_t nValuesPerSide, glm::vec3 center, double sideLen) {
  VolumetricGrid* s = new VolumetricGrid(name, field, nValuesPerSide, center, sideLen);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }
  return s;
}

template<typename Implicit>
VolumetricGrid* registerImplicitSurfaceFunction(std::string name, const Implicit &surface,
size_t nValuesPerSide, glm::vec3 center, double sideLen, bool meshImmediately = true) {
    size_t totalValues = nValuesPerSide * nValuesPerSide * nValuesPerSide;
    std::vector<double> field(totalValues);
    marchingcubes::SampleFunctionToGrid<Implicit>(surface, nValuesPerSide, center, sideLen, field);

    VolumetricGrid* outputSurface = registerImplicitSurfaceGrid(name, field, nValuesPerSide, center, sideLen);
    outputSurface->addGridIsosurfaceQuantity("isosurface", field);
    if (meshImmediately) {
      outputSurface->meshCurrentLevelSet();
    }
    return outputSurface;
}

} // namespace polyscope

