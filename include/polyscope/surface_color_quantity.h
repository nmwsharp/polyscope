// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/color_quantity.h"
#include "polyscope/render/engine.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

// forward declaration
class SurfaceMeshQuantity;
class SurfaceMesh;

class SurfaceColorQuantity : public SurfaceMeshQuantity, public ColorQuantity<SurfaceColorQuantity> {
public:
  SurfaceColorQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn,
                       const std::vector<glm::vec3>& colorValues);

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
  SurfaceVertexColorQuantity(std::string name, SurfaceMesh& mesh_, std::vector<glm::vec3> values_);

  virtual void createProgram() override;

  void buildVertexInfoGUI(size_t vInd) override;
};

// ========================================================
// ==========             Face Color             ==========
// ========================================================

class SurfaceFaceColorQuantity : public SurfaceColorQuantity {
public:
  SurfaceFaceColorQuantity(std::string name, SurfaceMesh& mesh_, std::vector<glm::vec3> values_);

  virtual void createProgram() override;

  void buildFaceInfoGUI(size_t fInd) override;
};

} // namespace polyscope
