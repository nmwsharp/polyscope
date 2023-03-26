// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/volume_grid_quantity.h"
#include "polyscope/volume_grid_scalar_quantity.h"
#include "polyscope/volume_grid_vector_quantity.h"

#include <vector>

// #include "marchingcubes/mesh_implicit_surface.h"

namespace polyscope {

class VolumeGrid;
class VolumeGridScalarIsosurface;
class VolumeGridScalarQuantity;
class VolumeGridVectorQuantity;

template <> // Specialize the quantity type
struct QuantityTypeHelper<VolumeGrid> {
  typedef VolumeGridQuantity type;
};


class VolumeGrid : public QuantityStructure<VolumeGrid> {
public:
  // Construct a new volume grid structure
  VolumeGrid(std::string name, std::array<size_t, 3> steps_, glm::vec3 bound_min_, glm::vec3 bound_max_);

  // === Overloads

  // Standard structure overrides
  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void drawPick() override;
  virtual void updateObjectSpaceBounds() override;
  virtual std::string typeName() override;
  virtual void refresh() override;

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildPickUI(size_t localPickID) override;

  // Field data
  std::array<size_t, 3> steps;
  glm::vec3 bound_min, bound_max;

  // Misc data
  static const std::string structureTypeName;

  // === Quantity-related
  // clang-format off


  template <class T>
  VolumeGridScalarQuantity* addScalarQuantity(std::string name, const T& values, DataType dataType_ = DataType::STANDARD);
  
  template <class Func>
  VolumeGridScalarQuantity* addScalarQuantityFromCallable(std::string name, Func&& func, DataType dataType_ = DataType::STANDARD);

  template <class Func>
  VolumeGridScalarQuantity* addScalarQuantityFromBatchCallable(std::string name, Func&& func, DataType dataType_ = DataType::STANDARD);

  template <class T>
  VolumeGridVectorQuantity* addVectorQuantity(std::string name, const T& vecValues, VectorType dataType_ = VectorType::STANDARD);

  //template <class T> VolumeGridScalarIsosurface* addGridIsosurfaceQuantity(std::string name, double isoLevel, const T& values);
  //template <class Funct> VolumeGridVectorQuantity* addGridVectorQuantityFromFunction(std::string name, const Funct& funct, VectorType dataType_);
  //template <class Funct> VolumeGridScalarQuantity* addGridScalarQuantityFromFunction(std::string name, const Funct& funct, DataType dataType_);
  
  // === Get/set visualization parameters

  // Material
  VolumeGrid* setMaterial(std::string name);
  std::string getMaterial();

  // Rendering helpers used by quantities
  std::vector<glm::vec3> gridPointLocations; // always populated
  void populateGeometry();
  void setVolumeGridUniforms(render::ShaderProgram& p);
  void setVolumeGridPointUniforms(render::ShaderProgram& p);
  std::vector<std::string> addVolumeGridPointRules(std::vector<std::string> initRules);
  
  // Helpers for computing with the grid
  size_t nValues() const;
  std::array<size_t, 3> flattenIndex(size_t i) const;
  glm::vec3 positionOfIndex(size_t i) const;
  glm::vec3 positionOfIndex(std::array<size_t, 3> inds) const;
  glm::vec3 gridSpacing() const;
  float minGridSpacing() const;


private:
  
  // === Visualization parameters
  PersistentValue<std::string> material;
  
  // === Quantity adder implementations
  // clang-format off
  
  VolumeGridScalarQuantity* addScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType dataType_);

  VolumeGridVectorQuantity* addVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& data, VectorType dataType_);
  // clang-format on
};


VolumeGrid* registerVolumeGrid(std::string name, std::array<size_t, 3> steps, glm::vec3 bound_min, glm::vec3 bound_max);
VolumeGrid* registerVolumeGrid(std::string name, size_t steps, glm::vec3 bound_min, glm::vec3 bound_max);

// Shorthand to get a point cloud from polyscope
inline VolumeGrid* getVolumeGrid(std::string name = "");
inline bool hasVolumeGrid(std::string name = "");
inline void removeVolumeGrid(std::string name = "", bool errorIfAbsent = false);

} // namespace polyscope

#include "polyscope/volume_grid.ipp"
