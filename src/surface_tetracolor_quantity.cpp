
#include "polyscope/surface_tetracolor_quantity.h"

#include "polyscope/polyscope.h"

namespace polyscope {

SurfaceTetracolorQuantity::SurfaceTetracolorQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn_,
                                                     const std::vector<glm::vec4>& tetracolorValues_)
  : SurfaceMeshQuantity(name, mesh_, true), TetracolorQuantity(*this, tetracolorValues_), definedOn(definedOn_) {}

void SurfaceTetracolorQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*program);
  parent.setSurfaceMeshUniforms(*program);
  
  render::engine->setBlendMode(BlendMode::Disable);
  program->draw();
}

std::string SurfaceTetracolorQuantity::niceName() {
  return name + " (" + definedOn + " tetracolor)";
}

void SurfaceTetracolorQuantity::refresh() {
  return;
}

// ========================================================
// ==========           Vertex Color            ===========
// ========================================================

SurfaceVertexTetracolorQuantity::SurfaceVertexTetracolorQuantity(std::string name, SurfaceMesh& mesh_,
                                                                 std::vector<glm::vec4> tetracolorValues_)
  : SurfaceTetracolorQuantity(name, mesh_, "vertex", tetracolorValues_) {}

void SurfaceVertexTetracolorQuantity::createProgram() {
  // Create the shader program to draw this quantity
  program = render::engine->requestShader("MESH_TETRA",
    render::engine->addMaterialRules("flat_tetra",
      addTetracolorRules(
        parent.addSurfaceMeshRules(
          {"MESH_PROPAGATE_TETRACOLOR", "SHADE_TETRACOLOR"}
        )
      )
    )
  );

  parent.setMeshGeometryAttributes(*program);
  program->setAttribute("a_tetracolor", tetracolors.getIndexedRenderAttributeBuffer(parent.triangleVertexInds));
}

// ========================================================
// ==========            Face Color              ==========
// ========================================================

SurfaceFaceTetracolorQuantity::SurfaceFaceTetracolorQuantity(std::string name, SurfaceMesh& mesh_,
                                                             std::vector<glm::vec4> tetracolorValues_)
  : SurfaceTetracolorQuantity(name, mesh_, "face", tetracolorValues_) {}

void SurfaceFaceTetracolorQuantity::createProgram() {
  // Create the shader program to draw this quantity
  program = render::engine->requestShader("MESH_TETRA",
    render::engine->addMaterialRules("flat_tetra",
      addTetracolorRules(
        parent.addSurfaceMeshRules(
          {"MESH_PROPAGATE_TETRACOLOR", "SHADE_TETRACOLOR"}
        )
      )
    )
  );

  parent.setMeshGeometryAttributes(*program);
  program->setAttribute("a_tetracolor", tetracolors.getIndexedRenderAttributeBuffer(parent.triangleFaceInds));
}

} // namespace polyscope
