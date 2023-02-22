#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/render/engine.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

// forward declaration
class SurfaceMeshQuantity;
class SurfaceMesh;

class SurfaceTextureQuantity : public SurfaceMeshQuantity {
public:
  SurfaceTextureQuantity(std::string name, std::vector<glm::vec2> uvs, const Texture& texture, SurfaceMesh& mesh);

  virtual void draw() override;
  virtual std::string niceName() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;

  template <class T> SurfaceTextureQuantity* setUVs(const T& uvs) {
    uvs_ = standardizeVectorArray<glm::vec2, 2>(uvs);
    createProgram();
    requestRedraw();
    return this;
  }

  SurfaceTextureQuantity* setTexture(const Texture& texture);

private:
  std::shared_ptr<render::ShaderProgram> program;
  std::vector<glm::vec2> uvs_;

  void createProgram();
  void setProgramUniforms(render::ShaderProgram& program);
  void fillColorBuffers(render::ShaderProgram& p);
};

} // namespace polyscope
