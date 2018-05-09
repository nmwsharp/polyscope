#pragma once

#include <vector>

#include "geometrycentral/geometry.h"
#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/vector3.h"

#include "polyscope/affine_remapper.h"
#include "polyscope/color_management.h"
#include "polyscope/gl/gl_utils.h"
#include "polyscope/structure.h"

namespace polyscope {

enum class ShadeStyle { FLAT = 0, SMOOTH };
enum class MeshElement { VERTEX = 0, FACE, EDGE, HALFEDGE };


// Forward delcare surface mesh
class SurfaceMesh;

// Data defined on a surface mesh
class SurfaceQuantity {
public:
  // Base constructor which sets the name
  SurfaceQuantity(std::string name, SurfaceMesh* mesh);
  virtual ~SurfaceQuantity() = 0;

  // Draw the quantity on the surface Note: for many quantities (like scalars)
  // this does nothing, because drawing happens in the mesh draw(). However
  // others (ie vectors) need to be drawn.
  virtual void draw() = 0;

  // Draw the ImGUI ui elements
  virtual void drawUI() = 0;

  // Build GUI info about this element
  virtual void buildInfoGUI(VertexPtr v);
  virtual void buildInfoGUI(FacePtr f);
  virtual void buildInfoGUI(EdgePtr e);
  virtual void buildInfoGUI(HalfedgePtr he);

  // === Member variables ===
  const std::string name;
  SurfaceMesh* const parent;

  bool enabled = false; // should be set by enable() and disable()
};

// Specific subclass indicating that a quantity can create a program to draw on
// the surface
class SurfaceQuantityThatDrawsFaces : public SurfaceQuantity {
public:
  SurfaceQuantityThatDrawsFaces(std::string name, SurfaceMesh* mesh);
  // Create a program to be used for drawing the surface
  // CALLER is responsible for deallocating
  virtual gl::GLProgram* createProgram() = 0;

  // Do any per-frame work on the program handed out by createProgram
  virtual void setProgramValues(gl::GLProgram* program);
};


class SurfaceMesh : public Structure {
public:
  // === Member functions ===

  // Construct a new surface mesh structure
  SurfaceMesh(std::string name, Geometry<Euclidean>* geometry_);
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
  virtual std::tuple<geometrycentral::Vector3, geometrycentral::Vector3> boundingBox() override;

  // === Quantity-related

  // general form
  void addSurfaceQuantity(SurfaceQuantity* quantity);
  void addSurfaceQuantity(SurfaceQuantityThatDrawsFaces* quantity);
  SurfaceQuantity* getSurfaceQuantity(std::string name, bool errorIfAbsent = true);

  // Scalars
  void addQuantity(std::string name, VertexData<double>& value, DataType type = DataType::STANDARD);
  void addQuantity(std::string name, FaceData<double>& value, DataType type = DataType::STANDARD);
  void addQuantity(std::string name, EdgeData<double>& value, DataType type = DataType::STANDARD);
  void addQuantity(std::string name, HalfedgeData<double>& value, DataType type = DataType::STANDARD);

  // Distance
  void addDistanceQuantity(std::string name, VertexData<double>& distances);
  void addSignedDistanceQuantity(std::string name, VertexData<double>& distances);

  // Colors
  void addColorQuantity(std::string name, VertexData<Vector3>& value);
  void addColorQuantity(std::string name, FaceData<Vector3>& value);

  // Counts/Singularities/Indices/Values on isolated vertices
  void addCountQuantity(std::string name, std::vector<std::pair<VertexPtr, int>>& values);
  void addCountQuantity(std::string name, std::vector<std::pair<FacePtr, int>>& values);
  void addIsolatedVertexQuantity(std::string name, std::vector<std::pair<VertexPtr, double>>& values);

  // Subsets
  void addSubsetQuantity(std::string name, EdgeData<char>& subset);

  // Selections
  void addVertexSelectionQuantity(std::string name, VertexData<char>& initialMembership);
  void addInputCurveQuantity(std::string name);


  // Vectors
  void addVectorQuantity(std::string name, VertexData<Vector3>& value, VectorType vectorType = VectorType::STANDARD);
  void addVectorQuantity(std::string name, FaceData<Vector3>& value, VectorType vectorType = VectorType::STANDARD);
  void addVectorQuantity(std::string name, FaceData<Complex>& value, int nSym = 1,
                         VectorType vectorType = VectorType::STANDARD);
  void addVectorQuantity(std::string name, VertexData<Complex>& value, int nSym = 1,
                         VectorType vectorType = VectorType::STANDARD);
  void addVectorQuantity(std::string name, EdgeData<double>& value);

  void removeQuantity(std::string name);
  void setActiveSurfaceQuantity(SurfaceQuantityThatDrawsFaces* q);
  void clearActiveSurfaceQuantity();
  void removeAllQuantities();

  // === Make a one-time selection
  geometrycentral::VertexPtr selectVertex();
  geometrycentral::FacePtr selectFace();

  // === Mutate
  
  void updateGeometryPositions(Geometry<Euclidean>* newGeometry);

  // === Helpers
  void deleteProgram();
  void fillGeometryBuffers();

  // === Member variables ===
  bool enabled = true;

  // The mesh (this is a copy of what was passed in)
  geometrycentral::HalfedgeMesh* mesh;
  geometrycentral::Geometry<Euclidean>* geometry;
  geometrycentral::HalfedgeMeshDataTransfer transfer;

  // The original mesh and geometry that were passed in
  geometrycentral::HalfedgeMesh* originalMesh;
  geometrycentral::Geometry<Euclidean>* originalGeometry;

  static const std::string structureTypeName;
  SubColorManager colorManager;

  // Scene transform
  glm::mat4 objectTransform = glm::mat4(1.0);
  void resetTransform();
  void centerBoundingBox();
  glm::mat4 getModelView();

  // Picking helpers
  // One of these will be non-null on return
  void getPickedElement(size_t localPickID, VertexPtr& vOut, FacePtr& fOut, EdgePtr& eOut, HalfedgePtr& heOut);
  // Returns the face ands coordinates in that face of the last pick. fOut == FacePtr() if not in any face. Note that
  // you may needed to update the pick data, beacuse this uses mouse coordinates from the current state but possibly old
  // pick lookup results.
  void getPickedFacePoint(FacePtr& fOut, Vector3& baryCoordOut);

private:
  // Quantities
  std::map<std::string, SurfaceQuantity*> quantities;

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
  VertexData<size_t> vInd;
  FaceData<size_t> fInd;
  EdgeData<size_t> eInd;
  HalfedgeData<size_t> heInd;
  void buildVertexInfoGui(VertexPtr v);
  void buildFaceInfoGui(FacePtr f);
  void buildEdgeInfoGui(EdgePtr e);
  void buildHalfedgeInfoGui(HalfedgePtr he);

  // Gui implementation details
  bool ui_smoothshade = true;
  
  // Drawing related things
  gl::GLProgram* program = nullptr;
  gl::GLProgram* pickProgram = nullptr;


  // === Helper functions
  void fillGeometryBuffersSmooth();
  void fillGeometryBuffersFlat();
  Vector2 projectToScreenSpace(Vector3 coord);
  bool screenSpaceTriangleTest(FacePtr f, Vector2 testCoords, Vector3& bCoordOut);
};

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
}
inline std::ostream& operator<<(std::ostream& out, const MeshElement value) {
  return out << getMeshElementTypeName(value);
}

} // namespace polyscope
