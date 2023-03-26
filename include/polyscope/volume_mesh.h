// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <memory>
#include <vector>

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/render/engine.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"
#include "polyscope/volume_mesh_quantity.h"

// Alllll the quantities
#include "polyscope/volume_mesh_color_quantity.h"
#include "polyscope/volume_mesh_scalar_quantity.h"
#include "polyscope/volume_mesh_vector_quantity.h"

namespace polyscope {

// Forward declarations for quantities
class VolumeMeshVertexColorQuantity;
class VolumeMeshCellColorQuantity;
class VolumeMeshVertexScalarQuantity;
class VolumeMeshCellScalarQuantity;
class VolumeMeshVertexVectorQuantity;
class VolumeMeshCellVectorQuantity;


template <> // Specialize the quantity type
struct QuantityTypeHelper<VolumeMesh> {
  typedef VolumeMeshQuantity type;
};


// === The grand volume mesh class

class VolumeMesh : public QuantityStructure<VolumeMesh> {
public:
  typedef VolumeMeshQuantity QuantityType;

  // === Member functions ===

  // Construct a new volume mesh structure
  VolumeMesh(std::string name, const std::vector<glm::vec3>& vertexPositions,
             const std::vector<std::array<uint32_t, 8>>& cellIndices);

  // TODO add constructors & adaptors without intermediate nested list

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildCustomOptionsUI() override;
  virtual void buildPickUI(size_t localPickID) override;

  // Render the the structure on screen
  virtual void draw() override;
  virtual void drawDelayed() override;
  virtual void drawPick() override;
  virtual void updateObjectSpaceBounds() override;
  virtual std::string typeName() override;
  virtual void refresh() override;

  // == Geometric quantities
  // (actually, these are wrappers around the private raw data members, but external users should interact with these
  // wrappers)

  // positions
  render::ManagedBuffer<glm::vec3> vertexPositions;

  // connectivity / indices
  render::ManagedBuffer<uint32_t> triangleVertexInds; // on the split, triangulated mesh [3 * nTriFace]
  render::ManagedBuffer<uint32_t> triangleFaceInds;   // on the split, triangulated mesh [3 * nTriFace]
  render::ManagedBuffer<uint32_t> triangleCellInds;   // on the split, triangulated mesh [3 * nTriFace]

  // internal triangle data for rendering
  render::ManagedBuffer<glm::vec3> baryCoord;  // on the split, triangulated mesh [3 * nTriFace]
  render::ManagedBuffer<glm::vec3> edgeIsReal; // on the split, triangulated mesh [3 * nTriFace]
  render::ManagedBuffer<float> faceType;       // on the split, triangulated mesh [3 * nTriFace]

  // other internally-computed geometry
  render::ManagedBuffer<glm::vec3> faceNormals;
  render::ManagedBuffer<glm::vec3> cellCenters;

  // === Quantity-related
  // clang-format off

  // = Scalars (expect scalar array)
  template <class T> VolumeMeshVertexScalarQuantity* addVertexScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 
  template <class T> VolumeMeshCellScalarQuantity* addCellScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 

  // = Colors (expect vec3 array)
  template <class T> VolumeMeshVertexColorQuantity* addVertexColorQuantity(std::string name, const T& data);
  template <class T> VolumeMeshCellColorQuantity* addCellColorQuantity(std::string name, const T& data);

  
	// = Vectors (expect vector array, inner type must be indexable with correct dimension (3 for extrinsic)
	template <class T> VolumeMeshVertexVectorQuantity* addVertexVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> VolumeMeshCellVectorQuantity* addCellVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD);

  // clang-format on

  // === Mutate
  template <class V>
  void updateVertexPositions(const V& newPositions);


  // === Indexing conventions & data

  std::vector<std::array<uint32_t, 8>> cells; // unused entries hold INVALID_IND

  // === Manage the mesh itself

  // Counts
  size_t nVertices() { return vertexPositions.size(); }
  size_t nCells() { return cells.size(); }

  // In these face counts, the shared face between two cells is counted twice. (really it should face-side or half-face
  // or something)
  size_t nFacesTriangulation() const { return nFacesTriangulationCount; }
  size_t nFaces() const { return nFacesCount; }

  // Derived geometric quantities
  std::vector<char> faceIsInterior; // a flat array whose order matches the iteration order of the mesh

  // = Mesh helpers
  VolumeCellType cellType(size_t i) const;
  void computeCounts();           // call to populate counts and indices
  void computeConnectivityData(); // call to populate indexing arrays
  std::vector<std::string> addVolumeMeshRules(std::vector<std::string> initRules, bool withSurfaceShade = true,
                                              bool isSlice = false);

  // Manage a separate tetrahedral representation used for volumetric visualizations
  // (for a pure-tet mesh this will be the same as the cells array)
  // TODO use a managed buffer for this
  std::vector<std::array<uint32_t, 4>> tets;
  size_t nTets();
  void computeTets();    // fills tet buffer
  void ensureHaveTets(); //  ensure the tet buffer is filled (but don't rebuild if already done)

  // === Member variables ===
  static const std::string structureTypeName;

  // === Getters and setters for visualization settings

  // Color of the mesh
  VolumeMesh* setColor(glm::vec3 val);
  glm::vec3 getColor();

  // Color of the interior faces of the mesh
  VolumeMesh* setInteriorColor(glm::vec3 val);
  glm::vec3 getInteriorColor();

  // Color of edges
  VolumeMesh* setEdgeColor(glm::vec3 val);
  glm::vec3 getEdgeColor();

  // Material
  VolumeMesh* setMaterial(std::string name);
  std::string getMaterial();

  // Width of the edges. Scaled such that 1 is a reasonable weight for visible edges, but values  1 can be used for
  // bigger edges. Use 0. to disable.
  VolumeMesh* setEdgeWidth(double newVal);
  double getEdgeWidth();

  VolumeMeshVertexScalarQuantity* getLevelSetQuantity();
  void setLevelSetQuantity(VolumeMeshVertexScalarQuantity* _levelSet);

  // Rendering helpers used by quantities
  void setVolumeMeshUniforms(render::ShaderProgram& p);
  void fillGeometryBuffers(render::ShaderProgram& p);
  void fillSliceGeometryBuffers(render::ShaderProgram& p);
  static const std::vector<std::vector<std::array<size_t, 3>>>& cellStencil(VolumeCellType type);

  // Slice plane listeners
  std::vector<polyscope::SlicePlane*> volumeSlicePlaneListeners;
  void addSlicePlaneListener(polyscope::SlicePlane* sp);
  void removeSlicePlaneListener(polyscope::SlicePlane* sp);
  void refreshVolumeMeshListeners();


private:
  // == Mesh geometry buffers
  // Storage for the managed buffers above. You should generally interact with these through the managed buffers, not
  // these members.

  // positions
  std::vector<glm::vec3> vertexPositionsData;

  // connectivity / indices
  std::vector<uint32_t> triangleVertexIndsData; // to the split, triangulated mesh
  std::vector<uint32_t> triangleFaceIndsData;   // to the split, triangulated mesh
  std::vector<uint32_t> triangleCellIndsData;   // to the split, triangulated mesh

  // internal triangle data for rendering
  std::vector<glm::vec3> baryCoordData;
  std::vector<glm::vec3> edgeIsRealData;
  std::vector<float> faceTypeData;

  // other internally-computed geometry
  std::vector<glm::vec3> faceNormalsData;
  std::vector<glm::vec3> cellCentersData;

  // Visualization settings
  PersistentValue<glm::vec3> color;
  PersistentValue<glm::vec3> interiorColor;
  PersistentValue<glm::vec3> edgeColor;
  PersistentValue<std::string> material;
  PersistentValue<float> edgeWidth;

  // Level sets
  // TODO: not currently really supported
  float activeLevelSetValue;
  VolumeMeshVertexScalarQuantity* activeLevelSetQuantity;

  // Do setup work related to drawing, including allocating openGL data
  void prepare();
  void preparePick();
  void geometryChanged();
  void recomputeGeometryIfPopulated();

  // Picking-related
  // Order of indexing: vertices, cells
  // Within each set, uses the implicit ordering from the mesh data structure
  // These starts are LOCAL indices, indexing elements only with the mesh
  size_t cellPickIndStart;
  void buildVertexInfoGui(size_t vInd);
  void buildCellInfoGUI(size_t cInd);

  /// == Compute indices & geometry data
  void computeFaceNormals();
  void computeCellCenters();

  // Gui implementation details

  // Drawing related things
  std::shared_ptr<render::ShaderProgram> program;
  std::shared_ptr<render::ShaderProgram> pickProgram;

  // Internal members
  size_t nFacesTriangulationCount = 0;
  size_t nFacesCount = 0;

  // === Helper functions

  // Initialization work
  void initializeMeshTriangulation();

  void fillGeometryBuffersFlat(render::ShaderProgram& p);

  // stencils for looping over cells
  // (each is a list of faces, which is itself a list of 1 or more triangles)
  // clang-format off
  static const std::vector<std::vector<std::array<size_t, 3>>> stencilTet;
  static const std::vector<std::vector<std::array<size_t, 3>>> stencilHex;
  static const std::array<std::array<size_t, 8>, 8> rotationMap;
  static const std::array<std::array<std::array<size_t, 4>, 6>, 4> diagonalMap;

  // clang-format off

  // === Quantity adders

  VolumeMeshVertexColorQuantity* addVertexColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  VolumeMeshCellColorQuantity* addCellColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  VolumeMeshVertexScalarQuantity* addVertexScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  VolumeMeshCellScalarQuantity* addCellScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  VolumeMeshVertexVectorQuantity* addVertexVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType);
  VolumeMeshCellVectorQuantity* addCellVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType);

  // === Helper implementations

  //void setVertexTangentBasisXImpl(const std::vector<glm::vec3>& vectors);
  //void setFaceTangentBasisXImpl(const std::vector<glm::vec3>& vectors);
  //  clang-format on
};

// Register functions
template <class V, class C>
VolumeMesh* registerTetMesh(std::string name, const V& vertexPositions, const C& tetIndices);
template <class V, class C>
VolumeMesh* registerHexMesh(std::string name, const V& vertexPositions, const C& hexIndices);
template <class V, class C>
VolumeMesh* registerVolumeMesh(std::string name, const V& vertexPositions, const C& hexIndices);
template <class V, class Ct, class Ch>
VolumeMesh* registerTetHexMesh(std::string name, const V& vertexPositions, const Ct& tetIndices, const Ch& hexIndices);


// Shorthand to get a mesh from polyscope
inline VolumeMesh* getVolumeMesh(std::string name = "");
inline bool hasVolumeMesh(std::string name = "");
inline void removeVolumeMesh(std::string name = "", bool errorIfAbsent = false);


} // namespace polyscope

#include "polyscope/volume_mesh.ipp"
