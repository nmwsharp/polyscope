// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/color_management.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

#include "polyscope/sparse_volume_grid_color_quantity.h"
#include "polyscope/sparse_volume_grid_quantity.h"
#include "polyscope/sparse_volume_grid_scalar_quantity.h"

#include <cstdint>
#include <numeric>
#include <vector>

namespace polyscope {

class SparseVolumeGrid;
class SparseVolumeGridCellScalarQuantity;
class SparseVolumeGridNodeScalarQuantity;
class SparseVolumeGridCellColorQuantity;
class SparseVolumeGridNodeColorQuantity;

struct SparseVolumeGridPickResult {
  SparseVolumeGridElement elementType;
  glm::ivec3 cellIndex;   // only populated if cell
  uint64_t cellFlatIndex; // only populated if cell
  glm::ivec3 nodeIndex;   // only populated if node
};

class SparseVolumeGrid : public Structure {
public:
  // Construct a new sparse volume grid structure
  // The origin is the NODE/CORNER orgin. That is, the cell 0,0,0, will have its lower-left corner sitting at this
  // origin. If you wish to specify the CENTER of the the 0,0,0 cell, you should pass (cellOrigin - 0.5 *
  // gridCellWidth).
  SparseVolumeGrid(std::string name, glm::vec3 origin, glm::vec3 gridCellWidth, std::vector<glm::ivec3> occupiedCells);

  // === Overloads

  // Standard structure overrides
  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void drawPick() override;
  virtual void drawPickDelayed() override;
  virtual void updateObjectSpaceBounds() override;
  virtual std::string typeName() override;
  virtual void refresh() override;

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildCustomOptionsUI() override;
  virtual void buildPickUI(const PickResult& result) override;

  // Misc data
  static const std::string structureTypeName;

  // === Geometry members
  render::ManagedBuffer<glm::vec3> cellPositions;
  render::ManagedBuffer<glm::ivec3> cellIndices;

  // === Grid info
  uint64_t nCells() const;
  glm::vec3 getOrigin() const;
  glm::vec3 getGridCellWidth() const;
  const std::vector<glm::ivec3>& getOccupiedCells() const;

  // === Node-valued data helpers
  // All lazily computed on first access.

  // Corner-to-node index buffers (one per corner, indexed by cell)
  // Corner c = dx*4 + dy*2 + dz maps cell[i] -> index in canonical node order
  render::ManagedBuffer<uint32_t> cornerNodeInds[8];

  uint64_t nNodes(); // NOTE: not populated until nodes are enabled (use ensureHaveCornerNodeIndices() to force this)
  const std::vector<glm::ivec3>& getCanonicalNodeInds();
  void ensureHaveCornerNodeIndices();

  // Helper for quantities. Given node-valued data with any order, re-orders and takes subsets to exactly match the node
  // data layout. The input may contain extra/repeated entries, and be in any order, so long as all required node values
  // are there. If entries are missing, an error is thrown. If the data was already in canonical order which is the
  // output, the out param nodeIndicesAreCanonical is set to true, and no reordering is done.
  template <typename T>
  std::vector<T> canonicalizeNodeValueArray(const std::string& quantityName, const std::vector<glm::ivec3>& nodeIndices,
                                            const std::vector<T>& nodeValues, bool& nodeIndicesAreCanonical);

  // === Quantities

  // Cell scalar. Values array must be passed in the same order as initial input cell list.
  template <class T>
  SparseVolumeGridCellScalarQuantity* addCellScalarQuantity(std::string name, const T& values,
                                                            DataType type = DataType::STANDARD);

  // Node scalar. Indices are _node_ indices; the nodes are a shifted sparse grid offset from the cell enumeration. For
  // a cell with indices ijk, its corrners are the nodes with indices [(i,j,k), (i,j,k+1), ..., (i+1,j+1,k+1)]. Node
  // values are passed via a paired set of arrays, giving the node index and node value for each. Node values may be
  // passed in any order, and having extra entries is fine too, as long as all required nodes values are present.
  template <class TI, class TV>
  SparseVolumeGridNodeScalarQuantity* addNodeScalarQuantity(std::string name, const TI& nodeIndices,
                                                            const TV& nodeValues, DataType type = DataType::STANDARD);

  // Cell color. Values array must be passed in the same order as initial input cell list.
  template <class T>
  SparseVolumeGridCellColorQuantity* addCellColorQuantity(std::string name, const T& colors);

  // Node color. Indices are _node_ indices; the nodes are a shifted sparse grid offset from the cell enumeration. For
  // a cell with indices ijk, its corrners are the nodes with indices [(i,j,k), (i,j,k+1), ..., (i+1,j+1,k+1)]. Node
  // values are passed via a paired set of arrays, giving the node index and node value for each. Node values may be
  // passed in any order, and having extra entries is fine too, as long as all required nodes values are present.
  template <class TI, class TC>
  SparseVolumeGridNodeColorQuantity* addNodeColorQuantity(std::string name, const TI& nodeIndices,
                                                          const TC& nodeColors);

  // Force the grid to act as if nodes are in use (enable them for picking)
  void markNodesAsUsed();

  // Get data related to picking/selection
  SparseVolumeGridPickResult interpretPickResult(const PickResult& result);

  // Rendering related helpers
  void setCellGeometryAttributes(render::ShaderProgram& p);
  std::vector<std::string> addSparseGridShaderRules(std::vector<std::string> initRules, bool pickOnly = false);
  void setSparseVolumeGridUniforms(render::ShaderProgram& p, bool pickOnly = false);

  // === Getters and setters

  // Color of the grid cubes
  SparseVolumeGrid* setColor(glm::vec3 val);
  glm::vec3 getColor();

  // Width of the edges. Scaled such that 1 is a reasonable weight for visible edges, but values >1 can be used for
  // bigger edges. Use 0. to disable edges.
  SparseVolumeGrid* setEdgeWidth(double newVal);
  double getEdgeWidth();

  // Color of edges
  SparseVolumeGrid* setEdgeColor(glm::vec3 val);
  glm::vec3 getEdgeColor();

  // Material
  SparseVolumeGrid* setMaterial(std::string name);
  std::string getMaterial();

  // Scaling factor for the size of the little cubes
  SparseVolumeGrid* setCubeSizeFactor(double newVal);
  double getCubeSizeFactor();

  // Render mode (gridcube or wireframe)
  SparseVolumeGrid* setRenderMode(SparseVolumeGridRenderMode mode);
  SparseVolumeGridRenderMode getRenderMode();

  // Wireframe radius as a node-relative value. 1.0 is a reasonable default size.
  SparseVolumeGrid* setWireframeRadius(double newVal);
  double getWireframeRadius();

  // Color used for wireframe rendering
  SparseVolumeGrid* setWireframeColor(glm::vec3 val);
  glm::vec3 getWireframeColor();

private:
  // Field data
  glm::vec3 origin;
  glm::vec3 gridCellWidth;

  // === Storage for managed quantities
  std::vector<glm::vec3> cellPositionsData;
  std::vector<glm::ivec3> cellIndicesData;

  // User-facing occupied cell indices (signed)
  std::vector<glm::ivec3> occupiedCellsData;

  // Canonical sorted node indices and corner index buffers (lazily computed)
  bool haveCornerNodeIndices = false;
  std::vector<glm::ivec3> canonicalNodeIndsData;
  std::vector<uint32_t> cornerNodeIndsData[8];

  // === Visualization parameters
  PersistentValue<glm::vec3> color;
  PersistentValue<float> edgeWidth;
  PersistentValue<glm::vec3> edgeColor;
  PersistentValue<std::string> material;
  PersistentValue<float> cubeSizeFactor;
  PersistentValue<SparseVolumeGridRenderMode> renderMode;
  PersistentValue<float> wireframeRadius;
  PersistentValue<glm::vec3> wireframeColor;

  // Compute cell positions and GPU indices from occupiedCellsData
  void computeCellPositions();

  // Compute canonical node indices and corner index buffers from occupiedCellsData
  void computeCornerNodeIndices();

  // Picking-related
  bool nodesHaveBeenUsed = false;
  void buildCellInfoGUI(const SparseVolumeGridPickResult& result);
  void buildNodeInfoGUI(const SparseVolumeGridPickResult& result);

  // Drawing related things
  std::shared_ptr<render::ShaderProgram> program;
  std::shared_ptr<render::ShaderProgram> pickProgram;

  // Wireframe render mode
  std::shared_ptr<render::ShaderProgram> wireframeNodeProgram;
  std::shared_ptr<render::ShaderProgram> wireframeEdgeProgram;
  void ensureWireframeProgramsPrepared();
  void buildWireframeGeometry(std::vector<glm::vec3>& nodePositions, std::vector<glm::vec3>& edgeTailPositions,
                              std::vector<glm::vec3>& edgeTipPositions);

  // === Helpers
  void checkForDuplicateCells();
  size_t findCellFlatIndex(glm::ivec3 cellInd3);
  size_t findNodeFlatIndex(glm::ivec3 nodeInd3);
  void ensureRenderProgramPrepared();
  void ensurePickProgramPrepared();

  // Quantity impl methods
  SparseVolumeGridCellScalarQuantity* addCellScalarQuantityImpl(std::string name, const std::vector<float>& data,
                                                                DataType type);
  SparseVolumeGridNodeScalarQuantity* addNodeScalarQuantityImpl(std::string name,
                                                                const std::vector<glm::ivec3>& nodeIndices,
                                                                const std::vector<float>& nodeValues, DataType type);
  SparseVolumeGridCellColorQuantity* addCellColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  SparseVolumeGridNodeColorQuantity* addNodeColorQuantityImpl(std::string name,
                                                              const std::vector<glm::ivec3>& nodeIndices,
                                                              const std::vector<glm::vec3>& nodeColors);
};


// Register a sparse volume grid
// The origin is the NODE/CORNER orgin. That is, the cell 0,0,0, will have its lower-left corner sitting at this
// origin. If you wish to specify the CENTER of the the 0,0,0 cell, you should pass (cellOrigin - 0.5 *
// gridCellWidth).
template <class T>
SparseVolumeGrid* registerSparseVolumeGrid(std::string name, glm::vec3 origin, glm::vec3 gridCellWidth,
                                           const T& occupiedCells);

// Register a sparse volume grid (non-templated overload)
// The origin is the NODE/CORNER orgin. That is, the cell 0,0,0, will have its lower-left corner sitting at this
// origin. If you wish to specify the CENTER of the the 0,0,0 cell, you should pass (cellOrigin - 0.5 *
// gridCellWidth).
SparseVolumeGrid* registerSparseVolumeGrid(std::string name, glm::vec3 origin, glm::vec3 gridCellWidth,
                                           const std::vector<glm::ivec3>& occupiedCells);

// Shorthand to get a sparse volume grid from polyscope
inline SparseVolumeGrid* getSparseVolumeGrid(std::string name = "");
inline bool hasSparseVolumeGrid(std::string name = "");
inline void removeSparseVolumeGrid(std::string name = "", bool errorIfAbsent = false);

} // namespace polyscope

#include "polyscope/sparse_volume_grid.ipp"
