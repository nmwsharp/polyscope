// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <memory>
#include <vector>

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"
#include "polyscope/volume_mesh_quantity.h"

// Alllll the quantities
/*
#include "polyscope/volume_color_quantity.h"
#include "polyscope/volume_count_quantity.h"
#include "polyscope/volume_scalar_quantity.h"
#include "polyscope/volume_vector_quantity.h"
*/


namespace polyscope {

// Forward declarations for quantities
/*
class VolumeVertexColorQuantity;
class VolumeFaceColorQuantity;
class VolumeVertexScalarQuantity;
class VolumeFaceScalarQuantity;
class VolumeEdgeScalarQuantity;
class VolumeHalfedgeScalarQuantity;
class VolumeVertexScalarQuantity;
class VolumeCornerParameterizationQuantity;
class VolumeVertexParameterizationQuantity;
class VolumeVertexVectorQuantity;
class VolumeFaceVectorQuantity;
class VolumeVertexIntrinsicVectorQuantity;
class VolumeFaceIntrinsicVectorQuantity;
class VolumeOneFormIntrinsicVectorQuantity;
class VolumeVertexCountQuantity;
class VolumeVertexIsolatedScalarQuantity;
class VolumeFaceCountQuantity;
class VolumeGraphQuantity;
*/


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
             const std::vector<std::array<int64_t, 8>>& cellIndices);

  // Build the imgui display
  virtual void buildCustomUI() override;
  virtual void buildCustomOptionsUI() override;
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

  virtual void refresh() override;
  virtual std::vector<std::string> addStructureRules(std::vector<std::string> initRules) override;

  // === Quantity-related
  // clang-format off
  /*

  // = Scalars (expect scalar array)
  template <class T> VolumeVertexScalarQuantity* addVertexScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 
  template <class T> VolumeFaceScalarQuantity* addFaceScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 
  template <class T> VolumeEdgeScalarQuantity* addEdgeScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 
  template <class T> VolumeHalfedgeScalarQuantity* addHalfedgeScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);

  // = Distance (expect scalar array)
  template <class T> VolumeVertexScalarQuantity* addVertexDistanceQuantity(std::string name, const T& data);
  template <class T> VolumeVertexScalarQuantity* addVertexSignedDistanceQuantity(std::string name, const T& data);

  // = Colors (expect vec3 array)
  template <class T> VolumeVertexColorQuantity* addVertexColorQuantity(std::string name, const T& data);
  template <class T> VolumeFaceColorQuantity* addFaceColorQuantity(std::string name, const T& data);
  
	// = Vectors (expect vector array, inner type must be indexable with correct dimension (3 for extrinsic, 2 for intrinsic) 
	template <class T> VolumeVertexVectorQuantity* addVertexVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> VolumeVertexVectorQuantity* addVertexVectorQuantity2D(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> VolumeFaceVectorQuantity* addFaceVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> VolumeFaceVectorQuantity* addFaceVectorQuantity2D(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 


  // = Counts/Values on isolated vertices (expect index/value pairs)
  VolumeVertexCountQuantity* addVertexCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>&
  values); 
	VolumeFaceCountQuantity* addFaceCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>&
  values); 
	VolumeVertexIsolatedScalarQuantity* addVertexIsolatedScalarQuantity(std::string name, const std::vector<std::pair<size_t, double>>& values);

  */
  // clang-format on

  // === Mutate
  template <class V>
  void updateVertexPositions(const V& newPositions);


  // === Indexing conventions

  // Permutation arrays. Empty == default ordering
  std::vector<size_t> vertexPerm;
  std::vector<size_t> facePerm;
  std::vector<size_t> edgePerm;

  /*

  // Set permutations
  template <class T>
  void setVertexPermutation(const T& perm, size_t expectedSize = 0);
  template <class T>
  void setFacePermutation(const T& perm, size_t expectedSize = 0);
  template <class T>
  void setEdgePermutation(const T& perm, size_t expectedSize = 0);
  template <class T>
  void setHalfedgePermutation(const T& perm, size_t expectedSize = 0);
  template <class T>
  void setCornerPermutation(const T& perm, size_t expectedSize = 0);
  template <class T>
  void setAllPermutations(const std::array<std::pair<T, size_t>, 5>& perms);
  */

  // Get the expected data length, either using the default convention or a permutation as above
  size_t vertexDataSize;
  size_t faceDataSize;
  size_t edgeDataSize;
  size_t halfedgeDataSize;
  size_t cornerDataSize;


  // === Manage the mesh itself

  // Core data
  std::vector<glm::vec3> vertices;
  std::vector<std::array<int64_t, 8>> cells; // holds unused indices hold INVALID_IND

  // Derived indices
  // std::vector<std::vector<size_t>> edgeIndices;
  // std::vector<std::vector<size_t>> halfedgeIndices;

  // Counts
  size_t nVertices() const { return vertices.size(); }
  size_t nCells() const { return cells.size(); }

  // size_t nFacesTriangulationCount = 0; TODO
  size_t nFacesCount = 0;
  // size_t nFacesTriangulation() const { return nFacesTriangulationCount; }
  size_t nFaces() const { return nFacesCount; }

  size_t nEdgesCount = 0;
  size_t nEdges() const { return nEdgesCount; }

  // Derived geometric quantities
  std::vector<double> cellAreas;
  std::vector<double> faceAreas;
  std::vector<double> vertexAreas;

  // = Mesh helpers
  VolumeCellType cellType(size_t i) const;
  void computeCounts();       // call to populate counts and indices
  void computeGeometryData(); // call to populate normals/areas/lengths

  // === Member variables ===
  static const std::string structureTypeName;

  // === Getters and setters for visualization settings

  // Color of the mesh
  VolumeMesh* setColor(glm::vec3 val);
  glm::vec3 getColor();

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

  // Rendering helpers used by quantities
  void setVolumeMeshUniforms(render::ShaderProgram& p);
  void fillGeometryBuffers(render::ShaderProgram& p);
  static const std::vector<std::vector<std::array<size_t, 3>>>& cellStencil(VolumeCellType type);

private:
  // Visualization settings
  PersistentValue<glm::vec3> color;
  PersistentValue<glm::vec3> edgeColor;
  PersistentValue<std::string> material;
  PersistentValue<float> edgeWidth;

  // Do setup work related to drawing, including allocating openGL data
  void prepare();
  void preparePick();
  void geometryChanged(); // call whenever geometry changed

  // Picking-related
  // Order of indexing: vertices, edges, faces, cells
  // Within each set, uses the implicit ordering from the mesh data structure
  // These starts are LOCAL indices, indexing elements only with the mesh
  size_t facePickIndStart, edgePickIndStart, cellPickIndStart;
  void buildVertexInfoGui(size_t vInd);
  void buildEdgeInfoGui(size_t eInd);
  void buildFaceInfoGui(size_t fInd);
  void buildCellInfoGUI(size_t cInd);

  // Gui implementation details

  // Drawing related things
  std::shared_ptr<render::ShaderProgram> program;
  std::shared_ptr<render::ShaderProgram> pickProgram;


  // === Helper functions

  // Initialization work
  void initializeMeshTriangulation();

  void fillGeometryBuffersFlat(render::ShaderProgram& p);

  // stencils for looping over cells
  // (each is a list of faces, which is itself a list of 1 or more triangles)
  // clang-format off
  static const std::vector<std::vector<std::array<size_t, 3>>> stencilTet;
  static const std::vector<std::vector<std::array<size_t, 3>>> stencilHex;

  // clang-format off

  // === Quantity adders

  /*
  VolumeVertexColorQuantity* addVertexColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  VolumeFaceColorQuantity* addFaceColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  VolumeVertexScalarQuantity* addVertexScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  VolumeFaceScalarQuantity* addFaceScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  VolumeEdgeScalarQuantity* addEdgeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  VolumeHalfedgeScalarQuantity* addHalfedgeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  VolumeVertexVectorQuantity* addVertexVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType);
  VolumeFaceVectorQuantity* addFaceVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType);
  VolumeVertexCountQuantity* addVertexCountQuantityImpl(std::string name, const std::vector<std::pair<size_t, int>>& values);
  VolumeVertexIsolatedScalarQuantity* addVertexIsolatedScalarQuantityImpl(std::string name, const std::vector<std::pair<size_t, double>>& values);
  VolumeFaceCountQuantity* addFaceCountQuantityImpl(std::string name, const std::vector<std::pair<size_t, int>>& values);
  */

  // === Helper implementations

  void setVertexTangentBasisXImpl(const std::vector<glm::vec3>& vectors);
  void setFaceTangentBasisXImpl(const std::vector<glm::vec3>& vectors);
  // clang-format on
};

// Register functions
template <class V, class C>
VolumeMesh* registerTetMesh(std::string name, const V& vertexPositions, const C& tetIndices);
template <class V, class C>
VolumeMesh* registerHexMesh(std::string name, const V& vertexPositions, const C& hexIndices);
template <class V, class Ct, class Ch>
VolumeMesh* registerTetHexMesh(std::string name, const V& vertexPositions, const Ct& tetIndices, const Ct& hexIndices);


// Shorthand to get a mesh from polyscope
inline VolumeMesh* getVolumeMesh(std::string name = "");
inline bool hasVolumeMesh(std::string name = "");
inline void removeVolumeMesh(std::string name = "", bool errorIfAbsent = true);


} // namespace polyscope

#include "polyscope/volume_mesh.ipp"
