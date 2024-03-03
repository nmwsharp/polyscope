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

#include <cstdint>
#include <vector>

namespace polyscope {

class VolumeGrid;
class VolumeGridNodeScalarQuantity;
class VolumeGridCellScalarQuantity;

template <> // Specialize the quantity type
struct QuantityTypeHelper<VolumeGrid> {
  typedef VolumeGridQuantity type;
};


class VolumeGrid : public QuantityStructure<VolumeGrid> {
public:
  // Construct a new volume grid structure
  VolumeGrid(std::string name, glm::uvec3 gridNodeDim_, glm::vec3 boundMin_, glm::vec3 boundMax_);

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
  virtual void buildCustomOptionsUI() override;
  virtual void buildPickUI(size_t localPickID) override;

  // Misc data
  static const std::string structureTypeName;

  // === Geometry members

  // The define the grid cube visualization
  // The are propeties of the resolution only, the geometry is in a reference space.
  render::ManagedBuffer<glm::vec3> gridPlaneReferencePositions;
  render::ManagedBuffer<glm::vec3> gridPlaneReferenceNormals;
  render::ManagedBuffer<int32_t> gridPlaneAxisInds;


  // === Quantity-related
  // clang-format off


  template <class T>
  VolumeGridNodeScalarQuantity* addNodeScalarQuantity(std::string name, const T& values, DataType dataType_ = DataType::STANDARD);
  
  template <class Func>
  VolumeGridNodeScalarQuantity* addNodeScalarQuantityFromCallable(std::string name, Func&& func, DataType dataType_ = DataType::STANDARD);
  
  template <class Func>
  VolumeGridNodeScalarQuantity* addNodeScalarQuantityFromBatchCallable(std::string name, Func&& func, DataType dataType_ = DataType::STANDARD);
  
  template <class T>
  VolumeGridCellScalarQuantity* addCellScalarQuantity(std::string name, const T& values, DataType dataType_ = DataType::STANDARD);
  
  template <class Func>
  VolumeGridCellScalarQuantity* addCellScalarQuantityFromCallable(std::string name, Func&& func, DataType dataType_ = DataType::STANDARD);
  
  template <class Func>
  VolumeGridCellScalarQuantity* addCellScalarQuantityFromBatchCallable(std::string name, Func&& func, DataType dataType_ = DataType::STANDARD);

  
  // Rendering helpers used by quantities
  // void populateGeometry();
  std::vector<std::string> addGridCubeRules(std::vector<std::string> initRules, bool withShade=true);
  void setVolumeGridUniforms(render::ShaderProgram& p);
  void setGridCubeUniforms(render::ShaderProgram& p, bool withShade=true);
  
  // == Helpers for computing with the grid
 
  uint64_t nNodes() const; // total number of nodes
  uint64_t nCells() const; // total number of cells
  glm::vec3 gridSpacing() const; // space between nodes/cells, in world units
  glm::vec3 gridSpacingReference() const; // space between nodes/cells, on [0,1]^3
  float minGridSpacing() const; // smallest coordinate of gridSpacing()

  // Field data
  glm::uvec3 getGridNodeDim() const;
  glm::uvec3 getGridCellDim() const;
  glm::vec3 getBoundMin() const;
  glm::vec3 getBoundMax() const;

  // nodes
  uint64_t flattenNodeIndex(glm::uvec3 inds) const;
  glm::uvec3 unflattenNodeIndex(uint64_t i) const;
  glm::vec3 positionOfNodeIndex(uint64_t i) const;
  glm::vec3 positionOfNodeIndex(glm::uvec3 inds) const;
  
  // cells
  uint64_t flattenCellIndex(glm::uvec3 inds) const;
  glm::uvec3 unflattenCellIndex(uint64_t i) const;
  glm::vec3 positionOfCellIndex(uint64_t i) const;
  glm::vec3 positionOfCellIndex(glm::uvec3 inds) const;

  // force the grid to act as if the specified elements are in use (aka enable them for picking, etc)
  void markNodesAsUsed();
  void markCellsAsUsed();

  // === Getters and setters for visualization settings

  // Color of the grid volume
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

  // Scaling factor for the size of the little cubes
  VolumeGrid* setCubeSizeFactor(double newVal);
  double getCubeSizeFactor();

private:
  
  // Field data
  glm::uvec3 gridNodeDim;
  glm::uvec3 gridCellDim;
  glm::vec3 boundMin, boundMax;
 
  // === Storage for managed quantities
  std::vector<glm::vec3> gridPlaneReferencePositionsData;
  std::vector<glm::vec3> gridPlaneReferenceNormalsData;
  std::vector<int32_t> gridPlaneAxisIndsData;

  // === Visualization parameters
  PersistentValue<glm::vec3> color;
  PersistentValue<glm::vec3> edgeColor;
  PersistentValue<std::string> material;
  PersistentValue<float> edgeWidth;
  PersistentValue<float> cubeSizeFactor;

  // == Compute indices & geometry data
  void computeGridPlaneReferenceGeometry();
  
  // Picking-related
  // Order of indexing: vertices, cells
  // Within each set, uses the implicit ordering from the mesh data structure
  // These starts are LOCAL indices, indexing elements only with the mesh
  size_t globalPickConstant = INVALID_IND_64;
  glm::vec3 pickColor;
  void buildNodeInfoGUI(size_t vInd);
  void buildCellInfoGUI(size_t cInd);
  bool nodesHaveBeenUsed = false;
  bool cellsHaveBeenUsed = false;
  

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
  
  VolumeGridNodeScalarQuantity* addNodeScalarQuantityImpl(std::string name, const std::vector<float>& data, DataType dataType_);
  VolumeGridCellScalarQuantity* addCellScalarQuantityImpl(std::string name, const std::vector<float>& data, DataType dataType_);

  // clang-format on
};


VolumeGrid* registerVolumeGrid(std::string name, glm::uvec3 gridNodeDim, glm::vec3 boundMin, glm::vec3 boundMax);
VolumeGrid* registerVolumeGrid(std::string name, uint64_t gridNodeAxesDim, glm::vec3 boundMin, glm::vec3 boundMax);

// Shorthand to get a point cloud from polyscope
inline VolumeGrid* getVolumeGrid(std::string name = "");
inline bool hasVolumeGrid(std::string name = "");
inline void removeVolumeGrid(std::string name = "", bool errorIfAbsent = false);

} // namespace polyscope

#include "polyscope/volume_grid.ipp"
