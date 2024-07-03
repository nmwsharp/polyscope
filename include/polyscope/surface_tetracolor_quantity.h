
#pragma once

#include "polyscope/tetracolor_quantity.h"
#include "polyscope/render/engine.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

// Forward declarations
class SurfaceMeshQuantity;
class SurfaceMesh;
class SurfaceParameterizationQuantity;

class SurfaceTetracolorQuantity : public SurfaceMeshQuantity, public TetracolorQuantity<SurfaceTetracolorQuantity> {
public:
  SurfaceTetracolorQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn_,
                            const std::vector<glm::vec4>& tetracolorValues_);

  virtual void draw() override;
  virtual std::string niceName() override;
  virtual void refresh() override;

protected:
  const std::string definedOn;
  std::shared_ptr<render::ShaderProgram> program;

  virtual void createProgram() = 0;
}; // class SurfaceColorQuantity

// ========================================================
// ==========           Vertex Color             ==========
// ========================================================

class SurfaceVertexTetracolorQuantity : public SurfaceTetracolorQuantity {
public:
  SurfaceVertexTetracolorQuantity(std::string name, SurfaceMesh& mesh_, std::vector<glm::vec4> tetracolorValues_);

  virtual void createProgram() override;
}; // class SurfaceVertexTetracolorQuantity

// ========================================================
// ==========             Face Color             ==========
// ========================================================

class SurfaceFaceTetracolorQuantity : public SurfaceTetracolorQuantity {
public:
  SurfaceFaceTetracolorQuantity(std::string name, SurfaceMesh& mesh_, std::vector<glm::vec4> tetracolorValues_);

  virtual void createProgram() override;
}; // class SurfaceFaceTetracolorQuantity

} // namespace polyscope
