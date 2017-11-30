#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

// Represents a general vector field associated with a surface mesh, including
// R3 fields in the ambient space and (TODO) R2 fields embedded in the surface
class SurfaceVectorQuantity : public SurfaceQuantity {
 public:
  SurfaceVectorQuantity(std::string name, VertexData<Vector3>& vectors_,
                        SurfaceMesh* mesh_,
                        VectorType vectorType_ = VectorType::STANDARD);
  
  SurfaceVectorQuantity(std::string name, FaceData<Vector3>& vectors_,
                        SurfaceMesh* mesh_,
                        VectorType vectorType_ = VectorType::STANDARD);

  virtual void draw() override;
  virtual void drawUI() override;

  // Do work shared between all constructors
  void finishConstructing();

  // === Members
  const VectorType vectorType;
  std::vector<Vector3> vectorRoots;
  std::vector<Vector3> vectors;
  float lengthMult; // longest vector will be this fraction of lengthScale (if not ambient)
  float radiusMult; // radius is this fraction of lengthScale
  std::array<float, 3> vectorColor;

  // The map that takes values to [0,1] for drawing
  AffineRemapper<Vector3> mapper;

  // UI internals
  const std::string definedOn;

  // GL things
  void prepare();
  gl::GLProgram* program = nullptr;
};

}  // namespace polyscope