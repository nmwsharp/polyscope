// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
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
#include "polyscope/surface_count_quantity.h"
#include "polyscope/surface_graph_quantity.h"
#include "polyscope/surface_parameterization_quantity.h"
#include "polyscope/surface_scalar_quantity.h"
#include "polyscope/surface_vector_quantity.h"
// #include "polyscope/surface_selection_quantity.h"
// #include "polyscope/surface_subset_quantity.h"


namespace polyscope {

// Forward declarations for quantities
class SurfaceVertexColorQuantity;
class SurfaceFaceColorQuantity;
class SurfaceVertexScalarQuantity;
class SurfaceFaceScalarQuantity;
class SurfaceEdgeScalarQuantity;
class SurfaceHalfedgeScalarQuantity;
class SurfaceVertexScalarQuantity;
class SurfaceCornerParameterizationQuantity;
class SurfaceVertexParameterizationQuantity;
class SurfaceVertexVectorQuantity;
class SurfaceFaceVectorQuantity;
class SurfaceVertexIntrinsicVectorQuantity;
class SurfaceFaceIntrinsicVectorQuantity;
class SurfaceOneFormIntrinsicVectorQuantity;
class SurfaceVertexCountQuantity;
class SurfaceVertexIsolatedScalarQuantity;
class SurfaceFaceCountQuantity;
class SurfaceGraphQuantity;


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

  // == Geometric quantities
  // (actually, these are wrappers around the private raw data members, but external users should interact with these
  // wrappers)

  // positions
  render::ManagedBuffer<glm::vec3> vertexPositions;

  // connectivity / indices
  render::ManagedBuffer<uint32_t> triangleVertexInds;      // on triangulated mesh [3 * nTriFace]
  render::ManagedBuffer<uint32_t> triangleFaceInds;        // on triangulated mesh [3 * nTriFace]
  render::ManagedBuffer<uint32_t> triangleCornerInds;      // on triangulated mesh [3 * nTriFace]
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
  render::ManagedBuffer<std::array<glm::vec3, 2>> faceTangentSpaces;        // set by user
  render::ManagedBuffer<std::array<glm::vec3, 2>> vertexTangentSpaces;      // set by user
  render::ManagedBuffer<std::array<glm::vec3, 2>> defaultFaceTangentSpaces; // automatically computed, used internally


  // === Quantity-related
  // clang-format off

  // = Scalars (expect scalar array)
  template <class T> SurfaceVertexScalarQuantity* addVertexScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 
  template <class T> SurfaceFaceScalarQuantity* addFaceScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 
  template <class T> SurfaceEdgeScalarQuantity* addEdgeScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD); 
  template <class T> SurfaceHalfedgeScalarQuantity* addHalfedgeScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);

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
  
	// = Vectors (expect vector array, inner type must be indexable with correct dimension (3 for extrinsic, 2 for intrinsic) 
	template <class T> SurfaceVertexVectorQuantity* addVertexVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> SurfaceVertexVectorQuantity* addVertexVectorQuantity2D(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> SurfaceFaceVectorQuantity* addFaceVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> SurfaceFaceVectorQuantity* addFaceVectorQuantity2D(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> SurfaceFaceIntrinsicVectorQuantity* addFaceIntrinsicVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T> SurfaceVertexIntrinsicVectorQuantity* addVertexIntrinsicVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD); 
	template <class T, class O> SurfaceOneFormIntrinsicVectorQuantity* addOneFormIntrinsicVectorQuantity(std::string name, const T& data, const O& orientations);

  // these are old versions kept only for backward-compatibility, nsym is no longer supported
  template <class T> SurfaceFaceIntrinsicVectorQuantity* addFaceIntrinsicVectorQuantity(std::string name, const T& vectors, int nSym, VectorType vectorType = VectorType::STANDARD); 
	template <class T> SurfaceVertexIntrinsicVectorQuantity* addVertexIntrinsicVectorQuantity(std::string name, const T& vectors, int nSym, VectorType vectorType = VectorType::STANDARD); 


  // = Counts/Values on isolated vertexPositions (expect index/value pairs)
  /* 
  SurfaceVertexCountQuantity* addVertexCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>&
  values); 
	SurfaceFaceCountQuantity* addFaceCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>&
  values); 
	SurfaceVertexIsolatedScalarQuantity* addVertexIsolatedScalarQuantity(std::string name, const std::vector<std::pair<size_t, double>>& values);
  */


  // = Misc quantities
  /*
  template <class P, class E>
  SurfaceGraphQuantity* addSurfaceGraphQuantity(std::string name, const P& nodes, const E& edges);
  template <class P, class E>
  SurfaceGraphQuantity* addSurfaceGraphQuantity2D(std::string name, const P& nodes, const E& edges);
  template <class P>
  SurfaceGraphQuantity* addSurfaceGraphQuantity(std::string name, const std::vector<P>& paths);
  template <class P>
  SurfaceGraphQuantity* addSurfaceGraphQuantity2D(std::string name, const std::vector<P>& paths);

  // = I/O Selections
  // template <class T>
  // void addVertexSelectionQuantity(std::string name, const T& initialMembership);
  // void addInputCurveQuantity(std::string name);
  */

  // clang-format on


  // === Make a one-time selection
  long long int selectVertex();
  // size_t selectFace();

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
  // template <class T>
  // void setVertexPermutation(const T& perm, size_t expectedSize = 0);
  // template <class T>
  // void setFacePermutation(const T& perm, size_t expectedSize = 0);
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

  size_t nEdgesCount = 0;                       // populating this is expensive...
  size_t nEdges() const { return nEdgesCount; } // WARNING: returns 0 until something involving edges has been done

  size_t nCornersCount = 0; // = nHalfedges = sum face degree
  size_t nCorners() const { return nCornersCount; }
  size_t nHalfedges() const { return nCornersCount; }

  // = Mesh helpers
  void nestedFacesToFlat(const std::vector<std::vector<size_t>>& nestedInds);
  void computeConnectivityData(); // call to populate counts and indices
  void checkTriangular();         // check if the mesh is triangular, print a helpful error if not

  // = Manifold connectivity
  // These are always defined on the triangulated mesh.
  // Not necessarily populated by default. Call ensureHaveManifoldConnectivity() to be sure they are populated.
  void ensureHaveManifoldConnectivity();
  // Halfedges are implicitly indexed in order on the triangulated face list
  // (note that this may not match the halfedge perm that the user specifies)
  std::vector<size_t> twinHalfedge; // for halfedge i, the index of a twin halfedge

  // == Manage tangent spaces

  // check if the user has set tangent spaces, and print a helpful error if not
  void checkHaveVertexTangentSpaces();
  void checkHaveFaceTangentSpaces();

  // Set tangent space coordinates for vertices
  template <class T>
  void setVertexTangentBasisX(const T& vectors);
  template <class T>
  void setVertexTangentBasisX2D(const T& vectors);

  // Set tangent space coordinates for faces
  template <class T>
  void setFaceTangentBasisX(const T& vectors);
  template <class T>
  void setFaceTangentBasisX2D(const T& vectors);

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

  // these two form a flattened list giving the polygons of the mesh
  std::vector<uint32_t> faceIndsStart;
  std::vector<uint32_t> faceIndsEntries;

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
  std::vector<std::array<glm::vec3, 2>> faceTangentSpacesData;
  std::vector<std::array<glm::vec3, 2>> vertexTangentSpacesData;
  std::vector<std::array<glm::vec3, 2>> defaultFaceTangentSpacesData;


  // Derived connectivity quantities
  bool halfedgesHaveBeenUsed = false;
  bool edgesHaveBeenUsed = false;
  void markEdgesAsUsed();
  void markHalfedgesAsUsed();


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
  void computeDefaultFaceTangentSpaces();

  // Picking-related
  // Order of indexing: vertexPositions, faces, edges, halfedges
  // Within each set, uses the implicit ordering from the mesh data structure
  // These starts are LOCAL indices, indexing elements only with the mesh
  size_t facePickIndStart, edgePickIndStart, halfedgePickIndStart;
  void buildVertexInfoGui(size_t vInd);
  void buildFaceInfoGui(size_t fInd);
  void buildEdgeInfoGui(size_t eInd);
  void buildHalfedgeInfoGui(size_t heInd);

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
  SurfaceVertexScalarQuantity* addVertexDistanceQuantityImpl(std::string name, const std::vector<double>& data);
  SurfaceVertexScalarQuantity* addVertexSignedDistanceQuantityImpl(std::string name, const std::vector<double>& data);
  SurfaceCornerParameterizationQuantity* addParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords, ParamCoordsType type);
  SurfaceVertexParameterizationQuantity* addVertexParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords, ParamCoordsType type);
  SurfaceVertexParameterizationQuantity* addLocalParameterizationQuantityImpl(std::string name, const std::vector<glm::vec2>& coords, ParamCoordsType type);
  SurfaceVertexVectorQuantity* addVertexVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType);
  SurfaceFaceVectorQuantity* addFaceVectorQuantityImpl(std::string name, const std::vector<glm::vec3>& vectors, VectorType vectorType);
  SurfaceFaceIntrinsicVectorQuantity* addFaceIntrinsicVectorQuantityImpl(std::string name, const std::vector<glm::vec2>& vectors, VectorType vectorType);
  SurfaceVertexIntrinsicVectorQuantity* addVertexIntrinsicVectorQuantityImpl(std::string name, const std::vector<glm::vec2>& vectors, VectorType vectorType);
  SurfaceOneFormIntrinsicVectorQuantity* addOneFormIntrinsicVectorQuantityImpl(std::string name, const std::vector<double>& data, const std::vector<char>& orientations);

  /*
  SurfaceVertexCountQuantity* addVertexCountQuantityImpl(std::string name, const std::vector<std::pair<size_t, int>>& values);
  SurfaceVertexIsolatedScalarQuantity* addVertexIsolatedScalarQuantityImpl(std::string name, const std::vector<std::pair<size_t, double>>& values);
  SurfaceFaceCountQuantity* addFaceCountQuantityImpl(std::string name, const std::vector<std::pair<size_t, int>>& values);
	SurfaceGraphQuantity* addSurfaceGraphQuantityImpl(std::string name, const std::vector<glm::vec3>& nodes, const std::vector<std::array<size_t, 2>>& edges);
  */


  // === Helper implementations

  void setVertexTangentBasisXImpl(const std::vector<glm::vec3>& vectors);
  void setFaceTangentBasisXImpl(const std::vector<glm::vec3>& vectors);
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
