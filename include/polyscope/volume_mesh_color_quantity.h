// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/render/engine.h"
#include "polyscope/volume_mesh.h"

namespace polyscope {

// forward declaration
class VolumeMeshQuantity;
class VolumeMesh;

class VolumeMeshColorQuantity : public VolumeMeshQuantity {
public:
  VolumeMeshColorQuantity(std::string name, VolumeMesh& mesh_, std::string definedOn);

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

class VolumeMeshVertexColorQuantity : public VolumeMeshColorQuantity {
public:
  VolumeMeshVertexColorQuantity(std::string name, std::vector<glm::vec3> values_, VolumeMesh& mesh_);

  virtual void createProgram() override;
  void fillColorBuffers(render::ShaderProgram& p);

  void buildVertexInfoGUI(size_t vInd) override;

  // === Members
  std::vector<glm::vec3> values;
};

// ========================================================
// ==========             Cell Color             ==========
// ========================================================

class VolumeMeshCellColorQuantity : public VolumeMeshColorQuantity {
public:
  VolumeMeshCellColorQuantity(std::string name, std::vector<glm::vec3> values_, VolumeMesh& mesh_);

  virtual void createProgram() override;
  void fillColorBuffers(render::ShaderProgram& p);

  void buildCellInfoGUI(size_t cInd) override;

  // === Members
  std::vector<glm::vec3> values;
};

} // namespace polyscope
