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
enum class MeshElement { VERTEX = 0, FACE, EDGE, HALFEDGE };


// Forward delcare surface mesh
class SurfaceMesh;

// Interface defining extra things that surface quantity needs to know how to do
class SurfaceQuantityInterface {
public:
  // Build GUI info about this element
  virtual void buildVertexInfoGUI(size_t vInd);
  virtual void buildFaceInfoGUI(size_t fInd);
  virtual void buildEdgeInfoGUI(size_t eInd);
  virtual void buildHalfedgeInfoGUI(size_t heInd);
};

// Specific subclass indicating that a quantity can create a program to draw on
// the surface
class SurfaceQuantityThatDrawsFaces : public Quantity<SurfaceMesh>, public SurfaceQuantityInterface {
public:
  SurfaceQuantityThatDrawsFaces(std::string name, SurfaceMesh* mesh);
  // Create a program to be used for drawing the surface
  // CALLER is responsible for deallocating
  virtual gl::GLProgram* createProgram() = 0;

  // Do any per-frame work on the program handed out by createProgram
  virtual void setProgramValues(gl::GLProgram* program);

  virtual void enable();
  virtual void disable();
};

// Triangulation of a (possibly polygonal) face
// Elements that don't come from the original mesh (eg, interior edges) denoted with INVALID_IND.
struct TriangulationFace {
  size_t faceInd;
  std::array<size_t, 3> vertexInds;
  std::array<size_t, 3> edgeInds;
  std::array<size_t, 3> halfedgeInds;
  std::array<size_t, 3> neighborFaceInds; // on the triangulation
};


class SurfaceMesh : public Structure {
public:
  // === Member functions ===

  // Construct a new surface mesh structure
  template <class V, class F>
  SurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices);
  // SurfaceMesh(std::string name, std::vector<glm::vec3> vertexPositions_, std::vector<std::vector<size_t>>
  // faceIndices_);

  ~SurfaceMesh();

  // Render the the structure on screen
  virtual void draw() override;

  // Do setup work related to drawing, including allocating openGL data
  virtual void prepare() override;
  virtual void preparePick() override;

  // Build the imgui display
  virtual void drawUI() override;
  virtual void drawPickUI(size_t localPickID) override;
  virtual void drawSharedStructureUI() override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<glm::vec3, glm::vec3> boundingBox() override;

  // === Quantity-related

  // general form
  void addSurfaceQuantity(SurfaceQuantity* quantity); // will be deleted internally when appropriate
  void addSurfaceQuantity(std::shared_ptr<SurfaceQuantity> quantity);
  std::shared_ptr<SurfaceQuantity> getSurfaceQuantity(std::string name, bool errorIfAbsent = true);

  // Scalars (expect double arrays)
  template <class T>
  void addVertexScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);
  template <class T>
  void addFaceScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);
  template <class T>
  void addEdgeScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);
  template <class T>
  void addHalfedgeScalarQuantity(std::string name, const T& data, DataType type = DataType::STANDARD);

  // Distance (expect double array)
  template <class T>
  void addDistanceQuantity(std::string name, const T& data);
  template <class T>
  void addSignedDistanceQuantity(std::string name, const T& data);

  // Colors (expect glm::vec3 array)
  template <class T>
  void addVertexColorQuantity(std::string name, const T& data);
  template <class T>
  void addFaceColorQuantity(std::string name, const T& data);

  // Counts/Values on isolated vertices (expect index/value pairs)
  void addVertexCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>& values);
  void addFaceCountQuantity(std::string name, const std::vector<std::pair<size_t, int>>& values);
  void addIsolatedVertexScalarQuantity(std::string name, const std::vector<std::pair<size_t, double>>& values);

  // Subsets (expect char array)
  template <class T>
  void addSubsetQuantity(std::string name, const T& subset);

  // Vectors (expect vector array, inner type must be indexable with correct dimension)
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


  // I/O Selections
  template <class T>
  void addVertexSelectionQuantity(std::string name, const T& initialMembership);
  // void addInputCurveQuantity(std::string name);


  void removeQuantity(std::string name);
  void setActiveSurfaceQuantity(SurfaceQuantityThatDrawsFaces* q);
  void clearActiveSurfaceQuantity();

  // === Make a one-time selection
  // size_t selectVertex();
  // size_t selectFace();

  // === Mutate
  void updateVertexPositions(const std::vector<glm::vec3>& newPositions);

  // === Helpers
  void deleteProgram();
  void fillGeometryBuffers();
  void setShadeStyle(ShadeStyle newShadeStyle);

  size_t nHalfedges() const { return mesh.nHalfedges(); }
  size_t nVertices() const { return mesh.nVertices(); }
  size_t nEdges() const { return mesh.nEdges(); }
  size_t nTriangulationEdges() const { return triMesh.nEdges(); }
  size_t nFaces() const { return mesh.nFaces(); }
  size_t nTriangulationFaces() const { return triMesh.nFaces(); }
  size_t nBoundaryLoops() const { return triMesh.nBoundaryLoops(); }
  size_t nImaginaryHalfedges() const { return mesh.nImaginaryHalfedges(); }
  size_t nTriangulationImaginaryHalfedges() const { return triMesh.nImaginaryHalfedges(); }

  // === Member variables ===
  bool enabled = true;

  // The mesh (possibly polgonal), as passed in by the user.
  HalfedgeMesh mesh;

  // A halfedge mesh, which is a triangulation of the input mesh.
  // Relationships to the original mesh are tracked internally.
  // Yes, keeping an entirely duplicated copy of the mesh is wasteful, but doing so simplifies logic all over the place.
  HalfedgeMesh triMesh;

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
  // Quantities
  std::map<std::string, std::shared_ptr<SurfaceQuantity>> quantities;

  // Visualization settings
  Color3f baseColor;
  Color3f surfaceColor;
  ShadeStyle shadeStyle = ShadeStyle::SMOOTH;
  bool showEdges = false;
  float edgeWidth = 0.0;

  SurfaceQuantityThatDrawsFaces* activeSurfaceQuantity = nullptr; // a quantity that is respondible for drawing on the
                                                                  // surface and overwrites `program` with its own
                                                                  // shaders


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
  gl::GLProgram* program = nullptr;
  gl::GLProgram* pickProgram = nullptr;


  // === Helper functions

  // Initialization work
  void initializeMeshTriangulation();

  void fillGeometryBuffersSmooth();
  void fillGeometryBuffersFlat();
  glm::vec2 projectToScreenSpace(glm::vec3 coord);
  // bool screenSpaceTriangleTest(size_t fInd, glm::vec2 testCoords, glm::vec3& bCoordOut);
};


// Shorthand to add a mesh to polyscope
template <class V, class F>
void registerSurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices,
                         bool replaceIfPresent = true) {
  SurfaceMesh* s = new SurfaceMesh(name, vertexPositions, faceIndices);
  bool success = registerStructure(s);
  if (!success) delete s;
}


// Shorthand to get a mesh from polyscope
inline SurfaceMesh* getSurfaceMesh(std::string name = "") {
  return dynamic_cast<SurfaceMesh*>(getStructure(SurfaceMesh::structureTypeName, name));
}


// Implementation of templated constructor
template <class V, class F>
SurfaceMesh::SurfaceMesh(std::string name, const V& vertexPositions, const F& faceIndices)
    : Structure(name, SurfaceMesh::structureTypeName), mesh(standardizeVectorArray<glm::vec3, V, 3>(vertexPositions),
                                                            standardizeNestedList<size_t, F>(faceIndices), false),
      triMesh(standardizeVectorArray<glm::vec3, V, 3>(vertexPositions), standardizeNestedList<size_t, F>(faceIndices),
              true) {

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
  }
  throw std::runtime_error("broken");
}
inline std::ostream& operator<<(std::ostream& out, const MeshElement value) {
  return out << getMeshElementTypeName(value);
}

} // namespace polyscope


// Additional includes for quantities
#include "polyscope/surface_color_quantity.h"
#include "polyscope/surface_count_quantity.h"
#include "polyscope/surface_distance_quantity.h"
#include "polyscope/surface_scalar_quantity.h"
#include "polyscope/surface_selection_quantity.h"
#include "polyscope/surface_subset_quantity.h"
#include "polyscope/surface_vector_quantity.h"
