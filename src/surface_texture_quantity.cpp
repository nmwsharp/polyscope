#include "polyscope/surface_texture_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "stb_image.h"

#include "imgui.h"

#include <filesystem>
#include <fstream>

using std::cout;
using std::endl;

namespace polyscope {

SurfaceTextureQuantity::SurfaceTextureQuantity(std::string name, std::vector<glm::vec2> uvs_, const Texture& texture, SurfaceMesh& mesh)
  : SurfaceMeshQuantity(name, mesh, true), uvs(std::move(uvs_)) {
    setTexture(texture);
  }

  SurfaceTextureQuantity::SurfaceTextureQuantity(std::string name, SurfaceParameterizationQuantity* surfaceParameterizationQuantity_, const Texture& texture, SurfaceMesh& mesh)
: SurfaceMeshQuantity(name, mesh, true) {
    surfaceParameterizationQuantity = surfaceParameterizationQuantity_;
    setTexture(texture);
}

void SurfaceTextureQuantity::draw() {
  if (!isEnabled()) return;

  // Set uniforms
  setProgramUniforms(*program);
  parent.setStructureUniforms(*program);
  parent.setSurfaceMeshUniforms(*program);

  program->draw();
}

void SurfaceTextureQuantity::createProgram() {
  std::shared_ptr<render::ShaderProgram> tmpProgram = render::engine->requestShader("MESH",
                                          parent.addSurfaceMeshRules({"MESH_PROPAGATE_VALUE2", "SHADE_TEXTURE2COLOR"}));
  if (program != nullptr) {
    // If exists, grab textures from previous shader program before clearing it.
    tmpProgram->copyTextures(program);
    program.reset();
  }

  program = std::move(tmpProgram);

  fillColorBuffers(*program);
  parent.fillGeometryBuffers(*program);

  render::engine->setMaterial(*program, parent.getMaterial());
}


// Update range uniforms
void SurfaceTextureQuantity::setProgramUniforms(render::ShaderProgram& program) {}

void SurfaceTextureQuantity::buildCustomUI() {}

void SurfaceTextureQuantity::refresh() {
  createProgram();
  Quantity::refresh();
}

std::string SurfaceTextureQuantity::niceName() { return name; }

void SurfaceTextureQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<glm::vec2> coordVal;
  coordVal.reserve(3 * parent.nFacesTriangulation());

  std::vector<glm::vec2> uvs_ = surfaceParameterizationQuantity ? surfaceParameterizationQuantity->getCoords() : uvs;

  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();

    // implicitly triangulate from root
    size_t vRoot = face[0];
    for (size_t j = 1; (j + 1) < D; j++) {
      size_t vB = face[j];
      size_t vC = face[(j + 1) % D];

      coordVal.push_back(uvs_[vRoot]);
      coordVal.push_back(uvs_[vB]);
      coordVal.push_back(uvs_[vC]);
    }
  }

  // Store data in buffers
  p.setAttribute("a_value2", coordVal);
}

SurfaceTextureQuantity* SurfaceTextureQuantity::setTexture(const Texture& texture) {
  program.reset();
  createProgram();
  program->setTexture2D("t_image", texture.data, texture.width, texture.height, true, true, true);

  requestRedraw();
  return this;
}

} // namespace polyscope
