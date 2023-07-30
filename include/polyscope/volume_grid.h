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
  VolumeGrid(std::string name, glm::uvec3 gridNodeDim_, glm::vec3 bound_min_, glm::vec3 bound_max_);

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
  glm::uvec3 gridNodeDim;
  glm::uvec3 gridCellDim;
  glm::vec3 bound_min, bound_max;

  // Misc data
  static const std::string structureTypeName;

  // === Geometry members
  render::ManagedBuffer<glm::vec3> cellCenters;
  render::ManagedBuffer<glm::uvec3> cellInds;


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
  
  // Rendering helpers used by quantities
  // void populateGeometry();
  std::vector<std::string> addGridCubeRules(std::vector<std::string> initRules);
  void setVolumeGridUniforms(render::ShaderProgram& p);
  // void setVolumeGridPointUniforms(render::ShaderProgram& p);
  void setGridCubeUniforms(render::ShaderProgram& p);
  // std::vector<std::string> addVolumeGridPointRules(std::vector<std::string> initRules);
  
  // == Helpers for computing with the grid
 
  // whole grid
  uint64_t nNodes() const;
  uint64_t nCells() const;
  glm::vec3 gridSpacing() const;
  float minGridSpacing() const;

  // nodes
  glm::uvec3 flattenNodeIndex(uint64_t i) const;
  glm::vec3 positionOfNodeIndex(uint64_t i) const;
  glm::vec3 positionOfNodeIndex(glm::uvec3 inds) const;
  
  // cells
  glm::uvec3 flattenCellIndex(uint64_t i) const;
  glm::vec3 positionOfCellIndex(uint64_t i) const;
  glm::vec3 positionOfCellIndex(glm::uvec3 inds) const;


  // === Getters and setters for visualization settings

  // Color of the mesh
  VolumeGrid* setColor(glm::vec3 val);
  glm::vec3 getColor();

  // Color of edges
  VolumeGrid* setEdgeColor(glm::vec3 val);
  glm::vec3 getEdgeColor();

  // Material
  VolumeGrid* setMaterial(std::string name);
  std::string getMaterial();

  // Width of the edges. Scaled such that 1 is a reasonable weight for visible edges, but values  1 can be used for
  // bigger edges. Use 0. to disable.
  VolumeGrid* setEdgeWidth(double newVal);
  double getEdgeWidth();


private:
 
  // === Storage for managed quantities
  std::vector<glm::vec3> cellCentersData;
  std::vector<glm::uvec3> cellIndsData;
  
  // === Visualization parameters
  PersistentValue<glm::vec3> color;
  PersistentValue<glm::vec3> edgeColor;
  PersistentValue<std::string> material;
  PersistentValue<float> edgeWidth;
  PersistentValue<float> cubeSizeFactor;
  
  // == Compute indices & geometry data
  void computeCellCenters();
  void computeCellInds();

  // Drawing related things
  // if nullptr, prepare() (resp. preparePick()) needs to be called
  std::shared_ptr<render::ShaderProgram> program;
  std::shared_ptr<render::ShaderProgram> pickProgram;

  // === Helpers
  
  // Do setup work related to drawing, including allocating openGL data
  void ensureGridCubeRenderProgramPrepared();
  void ensureGridCubePickProgramPrepared();
  
  // === Quantity adder implementations
  // clang-format off
  
  VolumeGridScalarQuantity* addScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType dataType_);

  VolumeGridVectorQuantity* addVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& data, VectorType dataType_);
  // clang-format on
};


VolumeGrid* registerVolumeGrid(std::string name, glm::uvec3 gridNodeDim, glm::vec3 bound_min, glm::vec3 bound_max);
VolumeGrid* registerVolumeGrid(std::string name, uint64_t gridNodeDim, glm::vec3 bound_min, glm::vec3 bound_max);

// Shorthand to get a point cloud from polyscope
inline VolumeGrid* getVolumeGrid(std::string name = "");
inline bool hasVolumeGrid(std::string name = "");
inline void removeVolumeGrid(std::string name = "", bool errorIfAbsent = false);

} // namespace polyscope

#include "polyscope/volume_grid.ipp"
