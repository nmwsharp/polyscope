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
class VolumetricGridScalarQuantity;

template <> // Specialize the quantity type
struct QuantityTypeHelper<VolumetricGrid> {
  typedef VolumetricGridQuantity type;
};


class VolumetricGrid : public QuantityStructure<VolumetricGrid> {
public:
  // Construct a new curve network structure
  VolumetricGrid(std::string name, size_t nValuesPerSide, glm::vec3 center, double sideLen);

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
  size_t nCornersPerSide;
  glm::vec3 gridCenter;
  double sideLength;

  inline size_t nValues() const { return nCornersPerSide * nCornersPerSide * nCornersPerSide; }

  inline glm::vec3 positionOfIndex(size_t i) {
    size_t nPerSlice = nCornersPerSide * nCornersPerSide;
    size_t z = i / nPerSlice;
    size_t i_in_slice = i % nPerSlice;
    size_t y = i_in_slice / nCornersPerSide;
    size_t x = i_in_slice % nCornersPerSide;

    double cellSize = sideLength / (nCornersPerSide - 1);
    double radius = sideLength / 2;
    glm::vec3 lowerCorner = gridCenter - glm::vec3{radius, radius, radius};
    return lowerCorner + glm::vec3{x * cellSize, y * cellSize, z * cellSize};
  }

  // Misc data
  static const std::string structureTypeName;

  template <class T>
  VolumetricGridScalarIsosurface* addGridIsosurfaceQuantity(std::string name, double isoLevel, const T& values) {
    validateSize(values, nValues(), "grid isosurface quantity " + name);
    return addIsosurfaceQuantityImpl(name, isoLevel, standardizeArray<double, T>(values));
  }
  template <class T>
  VolumetricGridScalarQuantity* addGridScalarQuantity(std::string name, const T& values, DataType dataType_) {
    validateSize(values, nValues(), "grid scalar quantity " + name);
    return addScalarQuantityImpl(name, standardizeArray<double, T>(values), dataType_);
  }
  template <class Implicit>
  VolumetricGridScalarQuantity* addGridScalarQuantityFromFunction(std::string name, const Implicit& funct, DataType dataType_) {
    size_t totalValues = nCornersPerSide * nCornersPerSide * nCornersPerSide;
    std::vector<double> field(totalValues);
    marchingcubes::SampleFunctionToGrid<Implicit>(funct, nCornersPerSide, gridCenter, sideLength, field);
    return addGridScalarQuantity(name, field, dataType_);
  }

  glm::vec3 getColor();

private:
  PersistentValue<glm::vec3> color;
  VolumetricGrid* setColor(glm::vec3 newVal);
  VolumetricGridScalarIsosurface* addIsosurfaceQuantityImpl(std::string name, double isoLevel,
                                                            const std::vector<double>& data);
  VolumetricGridScalarQuantity* addScalarQuantityImpl(std::string name, const std::vector<double>& data,
                                                      DataType dataType_);
};


VolumetricGrid* registerVolumetricGrid(std::string name, size_t nValuesPerSide, glm::vec3 center, double sideLen);

template <typename Implicit>
VolumetricGrid* registerIsosurfaceFromFunction(std::string name, const Implicit& funct, size_t nValuesPerSide,
                                               glm::vec3 center, double sideLen, bool meshImmediately = true) {
  size_t totalValues = nValuesPerSide * nValuesPerSide * nValuesPerSide;
  std::vector<double> field(totalValues);
  marchingcubes::SampleFunctionToGrid<Implicit>(funct, nValuesPerSide, center, sideLen, field);

  VolumetricGrid* outputSurface = registerVolumetricGrid(name, nValuesPerSide, center, sideLen);
  outputSurface->addGridIsosurfaceQuantity("isosurface", 0, field);
  return outputSurface;
}

} // namespace polyscope
