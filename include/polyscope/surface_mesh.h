#pragma once

#include <memory>
#include <vector>

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/halfedge_mesh.h"
#include "polyscope/polyscope.h"
#include "polyscope/standardize_data_array.h"
#include "polyscope/structure.h"

// Note: additional quantity includes at bottom

namespace polyscope {

enum class ShadeStyle { FLAT = 0, SMOOTH };
enum class MeshElement { VERTEX = 0, FACE, EDGE, HALFEDGE, CORNER };


// Forward delcare surface mesh
class SurfaceMesh;

// Extend Quantity<SurfaceMesh> to add a few extra functions
class SurfaceMeshQuantity : public Quantity<SurfaceMesh> {
public:
  SurfaceMeshQuantity(std::string name, SurfaceMesh& parentStructure, bool dominates = false);

public:
  // Notify that the geometry has changed
  virtual void geometryChanged();

  // Build GUI info about this element
  virtual void buildVertexInfoGUI(size_t vInd);
  virtual void buildFaceInfoGUI(size_t fInd);
  virtual void buildEdgeInfoGUI(size_t eInd);
  virtual void buildHalfedgeInfoGUI(size_t heInd);
};

template <> // Specialize the quantity type
struct QuantityTypeHelper<SurfaceMesh> {
  typedef SurfaceMeshQuantity type;
};


// === The grand surface mesh class

class SurfaceMesh : public QuantityStructure<SurfaceMesh> {
public:
  typedef SurfaceMeshQuantity QuantityType;

  // === Member functions ===

  // Construct a new surface mesh structure
  template <class V, class F>
  SurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices);

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

  // === Quantity-related

  // = Scalars (expect double arrays)
  template <class T>
  void addVertexScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);
  template <class T>
  void addFaceScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);
  template <class T>
  void addEdgeScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);
  template <class T>
  void addHalfedgeScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);

  // = Distance (expect double array)
  template <class T>
  void addVertexDistanceQuantity(std::string name, const T& data);
  template <class T>
  void addVertexSignedDistanceQuantity(std::string name, const T& data);

  // = Colors (expect glm::vec3 array)
  template <class T>
  void addVertexColorQuantity(std::string name, const T& data);
  template <class T>
  void addFaceColorQuantity(std::string name, const T& data);

  // = Counts/Values on isolated vertices (expect index/value pairs)
  void addVertexCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>& values);
  void addFaceCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>& values);
  void addIsolatedVertexScalarQuantity(std::string name, const std::vector<std::pair<size_t, double>>& values);

  // = Subsets (expect char array)
  // template <class T>
  // void addEdgeSubsetQuantity(std::string name, const T& subset);

  // = Vectors (expect vector array, inner type must be indexable with correct dimension)
  template <class T>
  void addVertexVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD);
  template <class T>
  void addFaceVectorQuantity(std::string name, const T& vectors, VectorType vectorType = VectorType::STANDARD);
  template <class T>
  void addFaceIntrinsicVectorQuantity(std::string name, const T& vectors, int nSym = 1,
                                      VectorType vectorType = VectorType::STANDARD);
  template <class T>
  void addVertexIntrinsicVectorQuantity(std::string name, const T& vectors, int nSym = 1,
                                        VectorType vectorType = VectorType::STANDARD);
  template <class T>
  void addOneFormIntrinsicVectorQuantity(std::string name, const T& data); // expects scalar


  // = I/O Selections
  template <class T>
  void addVertexSelectionQuantity(std::string name, const T& initialMembership);
  // void addInputCurveQuantity(std::string name);


  // === Make a one-time selection
  // size_t selectVertex();
  // size_t selectFace();

  // === Mutate
  void updateVertexPositions(const std::vector<glm::vec3>& newPositions);


  // === Indexing conventions

  // Permutation arrays. Empty == default ordering
  std::vector<size_t> vertexPerm;
  std::vector<size_t> facePerm;
  std::vector<size_t> edgePerm;
  std::vector<size_t> halfedgePerm;
  std::vector<size_t> cornerPerm;

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

  // Get the expected data length, either using the default convention or a permutation as above
  size_t vertexDataSize;
  size_t faceDataSize;
  size_t edgeDataSize;
  size_t halfedgeDataSize;
  size_t cornerDataSize;


  // === Helpers
  void fillGeometryBuffers(gl::GLProgram& p);
  void setMeshUniforms(gl::GLProgram& p);
  void setShadeStyle(ShadeStyle newShadeStyle);


  // === Manage the mesh itself

  // Core data
  std::vector<glm::vec3> vertices;
  std::vector<std::vector<size_t>> faces;

  // Derived indices
  std::vector<std::vector<size_t>> edgeIndices;
  std::vector<std::vector<size_t>> halfedgeIndices;

  // Counts
  size_t nVertices() const { return vertices.size(); }
  size_t nFaces() const { return faces.size(); }

  size_t nFacesTriangulationCount = 0;
  size_t nFacesTriangulation() const { return faces.size(); }

  size_t nEdgesCount = 0;
  size_t nEdges() const { return nEdgesCount; }

  size_t nCornersCount = 0; // = nHalfedges = sum face degree
  size_t nCorners() const { return nCornersCount; }
  size_t nHalfedges() const { return nCornersCount; }

  // Derived geometric quantities
  std::vector<glm::vec3> faceNormals;
  std::vector<glm::vec3> vertexNormals;
  std::vector<double> faceAreas;
  std::vector<double> vertexAreas;
  std::vector<double> edgeLengths;

  std::vector<std::array<glm::vec3, 2>> faceTangentSpaces;
  std::vector<std::array<glm::vec3, 2>> vertexTangentSpaces;


  // = Mesh helpers
  void computeCounts();       // call to populate counts and indices
  void computeGeometryData(); // call to populate normals/areas/lengths

  // if there are no tangent spaces, builds the default ones
  void ensureHaveFaceTangentSpaces();
  void ensureHaveVertexTangentSpaces();

  // TODO templated functions to set tangent spaces

  // === Member variables ===
  static const std::string structureTypeName;
  SubColorManager colorManager;

  // Picking helpers
  // One of these will be non-null on return
  // void getPickedElement(size_t localPickID, size_t& vOut, size_t& fOut, size_t& eOut, size_t& heOut);

  // Returns the face ands coordinates in that face of the last pick. fOut == FacePtr() if not in any face. Note that
  // you may needed to update the pick data, beacuse this uses mouse coordinates from the current state but possibly old
  // pick lookup results.
  // void getPickedFacePoint(FacePtr& fOut, glm::vec3& baryCoordOut);

private:
  // Visualization settings
  Color3f baseColor;
  Color3f surfaceColor;
  ShadeStyle shadeStyle = ShadeStyle::SMOOTH;
  bool showEdges = false;
  float edgeWidth = 0.0;

  // Do setup work related to drawing, including allocating openGL data
  void prepare();
  void preparePick();
  void geometryChanged(); // call whenever geometry changed

  // Picking-related
  // Order of indexing: vertices, faces, edges, halfedges
  // Within each set, uses the implicit ordering from the mesh data structure
  // Thest starts are LOCAL indices, indexing elements only with the mesh
  size_t facePickIndStart, edgePickIndStart, halfedgePickIndStart;
  void buildVertexInfoGui(size_t vInd);
  void buildFaceInfoGui(size_t fInd);
  void buildEdgeInfoGui(size_t eInd);
  void buildHalfedgeInfoGui(size_t heInd);

  // Gui implementation details
  bool ui_smoothshade = true;

  // Drawing related things
  std::unique_ptr<gl::GLProgram> program;
  std::unique_ptr<gl::GLProgram> pickProgram;

  // === Helper functions

  // Initialization work
  void initializeMeshTriangulation();

  void fillGeometryBuffersSmooth(gl::GLProgram& p);
  void fillGeometryBuffersFlat(gl::GLProgram& p);
  glm::vec2 projectToScreenSpace(glm::vec3 coord);
  // bool screenSpaceTriangleTest(size_t fInd, glm::vec2 testCoords, glm::vec3& bCoordOut);
};


// ==== Implementations of template functions


// Shorthand to add a mesh to polyscope
template <class V, class F>
SurfaceMesh* registerSurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices,
                                 bool replaceIfPresent = true) {
  SurfaceMesh* s = new SurfaceMesh(name, vertexPositions, faceIndices);
  bool success = registerStructure(s);
  if (!success) {
    safeDelete(s);
  }

  return s;
}

// Shorthand to add a mesh to polyscope while also setting permutations
template <class V, class F, class P>
SurfaceMesh* registerSurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices,
                                 const std::array<std::pair<P, size_t>, 5>& perms, bool replaceIfPresent = true) {

  SurfaceMesh* s = registerSurfaceMesh(name, vertexPositions, faceIndices, replaceIfPresent);

  if (s) {
    s->setAllPermutations(perms);
  }

  return s;
}


// Shorthand to get a mesh from polyscope
inline SurfaceMesh* getSurfaceMesh(std::string name = "") {
  return dynamic_cast<SurfaceMesh*>(getStructure(SurfaceMesh::structureTypeName, name));
}


// Implementation of templated constructor
template <class V, class F>
SurfaceMesh::SurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices)
    : QuantityStructure<SurfaceMesh>(name), vertices(standardizeVectorArray<glm::vec3, V, 3>(vertexPositions)),
      faces(standardizeNestedList<size_t, F>(faceIndices)) {

  computeCounts();
  computeGeometryData();

  // Colors
  baseColor = getNextStructureColor();
  surfaceColor = baseColor;
  colorManager = SubColorManager(baseColor);

  prepare();
  preparePick();
}

// Make mesh element type printable
inline std::string getMeshElementTypeName(MeshElement type) {
  switch (type) {
  case MeshElement::VERTEX:
    return "vertex";
  case MeshElement::FACE:
    return "face";
  case MeshElement::EDGE:
    return "edge";
  case MeshElement::HALFEDGE:
    return "halfedge";
  case MeshElement::CORNER:
    return "corner";
  }
  throw std::runtime_error("broken");
}
inline std::ostream& operator<<(std::ostream& out, const MeshElement value) {
  return out << getMeshElementTypeName(value);
}

} // namespace polyscope

#include "polyscope/surface_mesh.ipp"

// Alllll the quantities
#include "polyscope/surface_color_quantity.h"
#include "polyscope/surface_count_quantity.h"
#include "polyscope/surface_distance_quantity.h"
#include "polyscope/surface_scalar_quantity.h"
#include "polyscope/surface_vector_quantity.h"
/*
#include "polyscope/surface_selection_quantity.h"
#include "polyscope/surface_subset_quantity.h"
*/
