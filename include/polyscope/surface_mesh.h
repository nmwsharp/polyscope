// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <memory>
#include <vector>

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"
#include "polyscope/surface_mesh_quantity.h"
#include "polyscope/types.h"

// Alllll the quantities
#include "polyscope/surface_color_quantity.h"
#include "polyscope/surface_parameterization_quantity.h"
#include "polyscope/surface_scalar_quantity.h"
#include "polyscope/surface_vector_quantity.h"
#include "polyscope/utilities.h"

namespace polyscope {

// Forward declarations for quantities
class SurfaceVertexColorQuantity;
class SurfaceFaceColorQuantity;
class SurfaceVertexScalarQuantity;
class SurfaceFaceScalarQuantity;
class SurfaceEdgeScalarQuantity;
class SurfaceHalfedgeScalarQuantity;
class SurfaceVertexScalarQuantity;
class SurfaceCornerScalarQuantity;
class SurfaceCornerParameterizationQuantity;
class SurfaceVertexParameterizationQuantity;
class SurfaceVertexVectorQuantity;
class SurfaceFaceVectorQuantity;
class SurfaceVertexTangentVectorQuantity;
class SurfaceFaceTangentVectorQuantity;
class SurfaceOneFormTangentVectorQuantity;


template <> // Specialize the quantity type
struct QuantityTypeHelper<SurfaceMesh> {
  typedef SurfaceMeshQuantity type;
};


// === The grand surface mesh class

class SurfaceMesh : public QuantityStructure<SurfaceMesh> {
public:
  typedef SurfaceMeshQuantity QuantityType;

  // == Constructors

  // initializes members
  SurfaceMesh(std::string name);

  // From flattened list
  SurfaceMesh(std::string name, const std::vector<glm::vec3>& vertexPositions,
              const std::vector<uint32_t>& faceIndsEntries, const std::vector<uint32_t>& faceIndsStart);

  // Construct from a nested face list
  SurfaceMesh(std::string name, const std::vector<glm::vec3>& vertexPositions,
              const std::vector<std::vector<size_t>>& faceIndices);


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

  // Mesh connectivity
  // (end users probably should not mess with theses)
  std::vector<uint32_t> faceIndsStart;
  std::vector<uint32_t> faceIndsEntries;

  // == Geometric quantities
  // (actually, these are wrappers around the private raw data members, but external users should interact with these
  // wrappers)

  // positions
  render::ManagedBuffer<glm::vec3> vertexPositions;

  // connectivity / indices
  render::ManagedBuffer<uint32_t> triangleVertexInds; // on triangulated mesh [3 * nTriFace]
  render::ManagedBuffer<uint32_t> triangleFaceInds;   // on triangulated mesh [3 * nTriFace]
  render::ManagedBuffer<uint32_t> triangleCornerInds; // on triangulated mesh [3 * nTriFace]
  // these next 3 use the ***perm if it has been set
  render::ManagedBuffer<uint32_t> triangleAllEdgeInds;     // on triangulated mesh, all 3 [3 * 3 * nTriFace]
  render::ManagedBuffer<uint32_t> triangleAllHalfedgeInds; // on triangulated mesh, all 3 [3 * 3 * nTriFace]
  render::ManagedBuffer<uint32_t> triangleAllCornerInds;   // on triangulated mesh, all 3 [3 * 3 * nTriFace]

  // internal triangle data for rendering
  render::ManagedBuffer<glm::vec3> baryCoord;  // on the split, triangulated mesh [3 * nTriFace]
  render::ManagedBuffer<glm::vec3> edgeIsReal; // on the split, triangulated mesh [3 * nTriFace]

  // other internally-computed geometry
  render::ManagedBuffer<glm::vec3> faceNormals;
  render::ManagedBuffer<glm::vec3> faceCenters;
  render::ManagedBuffer<double> faceAreas;
  render::ManagedBuffer<glm::vec3> vertexNormals;
  render::ManagedBuffer<double> vertexAreas;
  // render::ManagedBuffer<double> edgeLengths;

  // tangent spaces
  render::ManagedBuffer<glm::vec3> defaultFaceTangentBasisX;
  render::ManagedBuffer<glm::vec3> defaultFaceTangentBasisY;


  // === Quantity-related
  // clang-format off

  // = Scalars (expect scalar array)
  template <class T> SurfaceVertexScalarQuantity* addVertexScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 
  template <class T> SurfaceFaceScalarQuantity* addFaceScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 
  template <class T> SurfaceEdgeScalarQuantity* addEdgeScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 
  template <class T> SurfaceHalfedgeScalarQuantity* addHalfedgeScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);
  template <class T> SurfaceCornerScalarQuantity* addCornerScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);

  // = Distance (expect scalar array)
  template <class T> SurfaceVertexScalarQuantity* addVertexDistanceQuantity(std::string name, const T& data);
  template <class T> SurfaceVertexScalarQuantity* addVertexSignedDistanceQuantity(std::string name, const T& data);

  // = Colors (expect vec3 array)
  template <class T> SurfaceVertexColorQuantity* addVertexColorQuantity(std::string name, const T& data);
  template <class T> SurfaceFaceColorQuantity* addFaceColorQuantity(std::string name, const T& data);
  
	// = Parameterizations (expect vec2 array)
  template <class T> SurfaceCornerParameterizationQuantity* addParameterizationQuantity(std::string name, const T& coords, ParamCoordsType type = ParamCoordsType::UNIT); 
	template <class T> SurfaceVertexParameterizationQuantity* addVertexParameterizationQuantity(std::string name, const T& coords, ParamCoordsType type = ParamCoordsType::UNIT);
  template <class T> SurfaceVertexParameterizationQuantity* addLocalParameterizationQuantity(std::string name, const T& coords, ParamCoordsType type = ParamCoordsType::WORLD);
  
	// = Vectors (expect vector array, inner type must be indexable with correct dimension (3 for extrinsic, 2 for tangent) 
	template <class T> SurfaceVertexVectorQuantity* addVertexVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> SurfaceVertexVectorQuantity* addVertexVectorQuantity2D(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> SurfaceFaceVectorQuantity* addFaceVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> SurfaceFaceVectorQuantity* addFaceVectorQuantity2D(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
  template <class T, class BX, class BY> SurfaceFaceTangentVectorQuantity* addFaceTangentVectorQuantity(std::string name, const T& vectors, const BX& basisX, const BY& basisY, int nSym = 1, VectorType vectorType = VectorType::STANDARD); 
	template <class T, class BX, class BY> SurfaceVertexTangentVectorQuantity* addVertexTangentVectorQuantity(std::string name, const T& vectors, const BX& basisX, const BY& basisY, int nSym = 1, VectorType vectorType = VectorType::STANDARD);
	template <class T, class O> SurfaceOneFormTangentVectorQuantity* addOneFormTangentVectorQuantity(std::string name, const T& data, const O& orientations);


  // clang-format on


  // === Make a one-time selection
  long long int selectVertex();

  // === Mutate

  // NOTE: these DO NOT automatically recompute der
  template <class V>
  void updateVertexPositions(const V& newPositions);
  template <class V>
  void updateVertexPositions2D(const V& newPositions2D);


  // === Indexing conventions

  // Permutation arrays. Empty == default ordering
  std::vector<size_t> edgePerm;
  std::vector<size_t> halfedgePerm;
  std::vector<size_t> cornerPerm;

  // Set permutations
  template <class T>
  void setEdgePermutation(const T& perm, size_t expectedSize = 0);
  template <class T>
  void setHalfedgePermutation(const T& perm, size_t expectedSize = 0);
  template <class T>
  void setCornerPermutation(const T& perm, size_t expectedSize = 0);
  template <class T>
  void setAllPermutations(const std::array<std::pair<T, size_t>, 3>& perms);

  template <class T> // deprecated, for backward compatability only
  void setAllPermutations(const std::array<std::pair<T, size_t>, 5>& perms);

  // Get the expected data length, either using the default convention or a permutation as above
  size_t vertexDataSize = INVALID_IND;
  size_t faceDataSize = INVALID_IND;
  size_t edgeDataSize = INVALID_IND;
  size_t halfedgeDataSize = INVALID_IND;
  size_t cornerDataSize = INVALID_IND;


  // === Helpers

  // === Manage the mesh itself

  // Counts
  size_t nVertices();
  size_t nFaces() const { return faceIndsStart.size() - 1; }

  size_t nFacesTriangulationCount = 0;
  size_t nFacesTriangulation() const { return nFacesTriangulationCount; }

  size_t nEdgesCount = INVALID_IND; // populating this is expensive...
  size_t nEdges();                  // NOTE causes population of nEdgesCount

  size_t nCornersCount = 0; // = nHalfedges = sum face degree
  size_t nCorners() const { return nCornersCount; }
  size_t nHalfedges() const { return nCornersCount; }

  // = Mesh helpers
  void nestedFacesToFlat(const std::vector<std::vector<size_t>>& nestedInds);
  void computeConnectivityData(); // call to populate counts and indices
  void checkTriangular();         // check if the mesh is triangular, print a helpful error if not

  // Force the mesh to act as if the specified elements are in use (aka enable them for picking, etc)
  void markEdgesAsUsed();
  void markHalfedgesAsUsed();
  void markCornersAsUsed();

  // = Manifold connectivity
  // These are always defined on the triangulated mesh.
  // Not necessarily populated by default. Call ensureHaveManifoldConnectivity() to be sure they are populated.
  void ensureHaveManifoldConnectivity();
  // Halfedges are implicitly indexed in order on the triangulated face list
  // (note that this may not match the halfedge perm that the user specifies)
  std::vector<size_t> twinHalfedge; // for halfedge i, the index of a twin halfedge

  static const std::string structureTypeName;

  // === Getters and setters for visualization settings

  // Color of the mesh
  SurfaceMesh* setSurfaceColor(glm::vec3 val);
  glm::vec3 getSurfaceColor();

  // Color of edges
  SurfaceMesh* setEdgeColor(glm::vec3 val);
  glm::vec3 getEdgeColor();

  // Material
  SurfaceMesh* setMaterial(std::string name);
  std::string getMaterial();

  // Backface color
  SurfaceMesh* setBackFaceColor(glm::vec3 val);
  glm::vec3 getBackFaceColor();

  // Width of the edges. Scaled such that 1 is a reasonable weight for visible edges, but values  1 can be used for
  // bigger edges. Use 0. to disable.
  SurfaceMesh* setEdgeWidth(double newVal);
  double getEdgeWidth();

  // Backface policy
  SurfaceMesh* setBackFacePolicy(BackFacePolicy newPolicy);
  BackFacePolicy getBackFacePolicy();

  // Face normal type
  SurfaceMesh* setShadeStyle(MeshShadeStyle newStyle);
  MeshShadeStyle getShadeStyle();

  // == Rendering helpers used by quantities

  // void fillGeometryBuffers(render::ShaderProgram& p);
  std::vector<std::string> addSurfaceMeshRules(std::vector<std::string> initRules, bool withMesh = true,
                                               bool withSurfaceShade = true);
  void setMeshGeometryAttributes(render::ShaderProgram& p);
  void setMeshPickAttributes(render::ShaderProgram& p);
  void setSurfaceMeshUniforms(render::ShaderProgram& p);


  // === ~DANGER~ experimental/unsupported functions

  // === DEPRECATED

  // Deprecated: use shadeType instead
  SurfaceMesh* setSmoothShade(bool isSmooth);
  bool isSmoothShade();


private:
  // == Mesh geometry buffers
  // Storage for the managed buffers above. You should generally interact with these through the managed buffers, not
  // these members.

  // = positions
  std::vector<glm::vec3> vertexPositionsData;

  // = connectivity / indices

  // other derived indices, all defined per corner of the triangulated mesh
  std::vector<uint32_t> triangleVertexIndsData;      // index of the corresponding vertex
  std::vector<uint32_t> triangleFaceIndsData;        // index of the corresponding original face
  std::vector<uint32_t> triangleCornerIndsData;      // index of the corresponding original corner
  std::vector<uint32_t> triangleAllEdgeIndsData;     // index of the corresponding original edge
  std::vector<uint32_t> triangleAllHalfedgeIndsData; // index of the corresponding original halfedge
  std::vector<uint32_t> triangleAllCornerIndsData;   // index of the corresponding original corner

  // internal triangle data for rendering, defined per corner of the triangulated mesh
  std::vector<glm::vec3> baryCoordData;  // always triangulated
  std::vector<glm::vec3> edgeIsRealData; // always triangulated

  // other internally-computed geometry
  std::vector<glm::vec3> faceNormalsData;
  std::vector<glm::vec3> faceCentersData;
  std::vector<double> faceAreasData;
  std::vector<glm::vec3> vertexNormalsData;
  std::vector<double> vertexAreasData;
  // std::vector<double> edgeLengthsData;

  // tangent spaces
  std::vector<glm::vec3> defaultFaceTangentBasisXData;
  std::vector<glm::vec3> defaultFaceTangentBasisYData;


  // Derived connectivity quantities
  bool halfedgesHaveBeenUsed = false;
  bool cornersHaveBeenUsed = false;
  bool edgesHaveBeenUsed = false;
  std::vector<uint32_t>
      halfedgeEdgeCorrespondence; // ugly hack used to save a pick buffer attr, filled out lazily w/ edge indices


  // Visualization settings
  PersistentValue<glm::vec3> surfaceColor;
  PersistentValue<glm::vec3> edgeColor;
  PersistentValue<std::string> material;
  PersistentValue<float> edgeWidth;
  PersistentValue<BackFacePolicy> backFacePolicy;
  PersistentValue<glm::vec3> backFaceColor;
  PersistentValue<MeshShadeStyle> shadeStyle;

  // Do setup work related to drawing, including allocating openGL data
  void prepare();
  void preparePick();


  /// == Compute indices & geometry data
  void computeTriangleCornerInds();
  void computeTriangleAllEdgeInds();
  void computeTriangleAllHalfedgeInds();
  void computeTriangleAllCornerInds();
  void computeFaceNormals();
  void computeFaceCenters();
  void computeFaceAreas();
  void computeVertexNormals();
  void computeVertexAreas();
  void computeEdgeLengths();
  void computeDefaultFaceTangentBasisX();
  void computeDefaultFaceTangentBasisY();
  void countEdges();

  // Picking-related
  // Order of indexing: vertexPositions, faces, edges, halfedges
  // Within each set, uses the implicit ordering from the mesh data structure
  // These starts are LOCAL indices, indexing elements only with the mesh
  size_t facePickIndStart, edgePickIndStart, halfedgePickIndStart, cornerPickIndStart;
  void buildVertexInfoGui(size_t vInd);
  void buildFaceInfoGui(size_t fInd);
  void buildEdgeInfoGui(size_t eInd);
  void buildHalfedgeInfoGui(size_t heInd);
  void buildCornerInfoGui(size_t heInd);

  // ==== Gui implementation details

  std::shared_ptr<render::ShaderProgram> program;
  std::shared_ptr<render::ShaderProgram> pickProgram;


  // === Helper functions

  void initializeMeshTriangulation();
  void recomputeGeometryIfPopulated();

  glm::vec2 projectToScreenSpace(glm::vec3 coord);


  // clang-format off

  // === Quantity adders

  SurfaceVertexColorQuantity* addVertexColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  SurfaceFaceColorQuantity* addFaceColorQuantityImpl(std::string name, const std::vector<glm::vec3>& colors);
  SurfaceVertexScalarQuantity* addVertexScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  SurfaceFaceScalarQuantity* addFaceScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  SurfaceEdgeScalarQuantity* addEdgeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  SurfaceHalfedgeScalarQuantity* addHalfedgeScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  SurfaceCornerScalarQuantity* addCornerScalarQuantityImpl(std::string name, const std::vector<double>& data, DataType type);
  SurfaceVertexScalarQuantity* addVertexDistanceQuantityImpl(std::string name, const std::vector<double>& data);
  SurfaceVertexScalarQuantity* addVertexSignedDistanceQuantityImpl(std::string name, const std::vector<double>& data);
  SurfaceCornerParameterizationQuantity* addParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords, ParamCoordsType type);
  SurfaceVertexParameterizationQuantity* addVertexParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords, ParamCoordsType type);
  SurfaceVertexParameterizationQuantity* addLocalParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords, ParamCoordsType type);
  SurfaceVertexVectorQuantity* addVertexVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType);
  SurfaceFaceVectorQuantity* addFaceVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType);
  SurfaceFaceTangentVectorQuantity* addFaceTangentVectorQuantityImpl(std::string name, const std::vector<glm::vec2>& vectors, const std::vector<glm::vec3>& basisX, const std::vector<glm::vec3>& basisY, int nSym, VectorType vectorType);
  SurfaceVertexTangentVectorQuantity* addVertexTangentVectorQuantityImpl(std::string name, const std::vector<glm::vec2>& vectors, const std::vector<glm::vec3>& basisX, const std::vector<glm::vec3>& basisY, int nSym, VectorType vectorType);
  SurfaceOneFormTangentVectorQuantity* addOneFormTangentVectorQuantityImpl(std::string name, const std::vector<double>& data, const std::vector<char>& orientations);

  // === Helper implementations

  // clang-format on
};

// Register functions
template <class V, class F>
SurfaceMesh* registerSurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices);
template <class V, class F>
SurfaceMesh* registerSurfaceMesh2D(std::string name, const V& vertexPositions, const F& faceIndices);
template <class V, class F, class P>
SurfaceMesh* registerSurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices,
                                 const std::array<std::pair<P, size_t>, 5>& perms);


// Shorthand to get a mesh from polyscope
inline SurfaceMesh* getSurfaceMesh(std::string name = "");
inline bool hasSurfaceMesh(std::string name = "");
inline void removeSurfaceMesh(std::string name = "", bool errorIfAbsent = false);


} // namespace polyscope

#include "polyscope/surface_mesh.ipp"
