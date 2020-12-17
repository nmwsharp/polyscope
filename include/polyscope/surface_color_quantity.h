// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/render/engine.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

// forward declaration
class SurfaceMeshQuantity;
class SurfaceMesh;

class SurfaceColorQuantity : public SurfaceMeshQuantity {
public:
  SurfaceColorQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn);

  virtual void draw() override;
  virtual std::string niceName() override;

  virtual void refresh() override;

protected:
  // UI internals
  const std::string definedOn;
  std::shared_ptr<render::ShaderProgram> program;

  // Helpers
  virtual void createProgram() = 0;
};

// ========================================================
// ==========           Vertex Color             ==========
// ========================================================

class SurfaceVertexColorQuantity : public SurfaceColorQuantity {
public:
  SurfaceVertexColorQuantity(std::string name, std::vector<glm::vec3> values_, SurfaceMesh& mesh_);

  virtual void createProgram() override;
  void fillColorBuffers(render::ShaderProgram& p);

  void buildVertexInfoGUI(size_t vInd) override;

  // === Members
  std::vector<glm::vec3> values;
};

// ========================================================
// ==========             Face Color             ==========
// ========================================================

class SurfaceFaceColorQuantity : public SurfaceColorQuantity {
public:
  SurfaceFaceColorQuantity(std::string name, std::vector<glm::vec3> values_, SurfaceMesh& mesh_);

  virtual void createProgram() override;
  void fillColorBuffers(render::ShaderProgram& p);

  void buildFaceInfoGUI(size_t fInd) override;

  // === Members
  std::vector<glm::vec3> values;
};

} // namespace polyscope
