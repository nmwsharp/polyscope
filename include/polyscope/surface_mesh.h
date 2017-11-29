#pragma once

#include <vector>

#include "geometrycentral/vector3.h"
#include "geometrycentral/halfedge_mesh.h"
#include "geometrycentral/geometry.h"

#include "polyscope/gl/gl_utils.h"
#include "polyscope/structure.h"

namespace polyscope {

enum class ShadeStyle {FLAT=0, SMOOTH};

class SurfaceMesh : public Structure {
 public:
  // === Member functions ===

  // Construct a new surface mesh structure
  SurfaceMesh(std::string name, Geometry<Euclidean>* geometry_);

  // Render the the structure on screen
  virtual void draw() override;

  // Do setup work related to drawing, including allocating openGL data
  virtual void prepare() override;

  // Undo anything done in prepare(), including deallocating openGL data
  virtual void teardown() override;

  // Build the imgui display
  virtual void drawUI() override;

  // Render for picking
  virtual void drawPick() override;

  // A characteristic length for the structure
  virtual double lengthScale() override;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
  boundingBox() override;

  bool enabled = true;
  bool prepared = false;

 private:

  // The mesh
  geometrycentral::HalfedgeMesh* mesh;
  geometrycentral::Geometry<Euclidean>* geometry;
  geometrycentral::HalfedgeMeshDataTransfer transfer;

  // Visualization settings
  std::array<float, 3> surfaceColor;
  ShadeStyle shadeStyle = ShadeStyle::SMOOTH;
  bool showEdges = false;
  float edgeWidth = 0.0; // currently can only be set to 0 or nonzero via UI

  // Gui implementation details
  bool ui_smoothshade;

  // Drawing related things
  gl::GLProgram* program = nullptr;


  // === Helper functions
  void fillGeometryBuffersSmooth();
  void fillGeometryBuffersFlat();

};

}  // namespace polyscope