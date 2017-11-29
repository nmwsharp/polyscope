#pragma once

#include <vector>

#include "geometrycentral/geometry.h"
#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/vector3.h"

#include "polyscope/gl/gl_utils.h"
#include "polyscope/structure.h"

namespace polyscope {

enum class ShadeStyle { FLAT = 0, SMOOTH };
enum class DataType { STANDARD = 0, SYMMETRIC };

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

  // === Member variables ===
  const std::string name;
  SurfaceMesh* const parent;

  bool enabled = false;  // should be set by enable() and disable()
};

// Specific subclass indicating that a quantity can create a program to draw on
// the surface
class SurfaceQuantityThatDrawsFaces : public SurfaceQuantity {
 public:
  SurfaceQuantityThatDrawsFaces(std::string name, SurfaceMesh* mesh);
  // Create a program to be used for drawing the surface
  // CALLER is responsible for deallocating
  virtual gl::GLProgram* createProgram() = 0;
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

  // Build the imgui display
  virtual void drawUI() override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
  boundingBox() override;

  // === Quantity-related
  void addQuantity(std::string name, VertexData<double>& value);

  void setActiveSurfaceQuantity(SurfaceQuantityThatDrawsFaces* q);
  void clearActiveSurfaceQuantity();

  // === Helpers
  void deleteProgram();
  void fillGeometryBuffers();

  // === Member variables ===
  bool enabled = true;

  // The mesh
  geometrycentral::HalfedgeMesh* mesh;
  geometrycentral::Geometry<Euclidean>* geometry;
  geometrycentral::HalfedgeMeshDataTransfer transfer;

  // Drawing related things
  gl::GLProgram* program = nullptr;

 private:
  // Quantities
  std::map<std::string, SurfaceQuantity*> quantities;

  // Visualization settings
  std::array<float, 3> surfaceColor;
  ShadeStyle shadeStyle = ShadeStyle::SMOOTH;
  bool showEdges = false;
  float edgeWidth = 0.0;  // currently can only be set to 0 or nonzero via UI
  SurfaceQuantityThatDrawsFaces* activeSurfaceQuantity =
      nullptr;  // a quantity that is respondible for drawing on the surface and
                // overwrites `program` with its own shaders

  // Gui implementation details
  bool ui_smoothshade;

  // === Helper functions
  void fillGeometryBuffersSmooth();
  void fillGeometryBuffersFlat();
};

}  // namespace polyscope