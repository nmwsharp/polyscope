// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/color_quantity.h"
#include "polyscope/render/engine.h"
#include "polyscope/volume_mesh.h"

namespace polyscope {

// forward declaration
class VolumeMeshQuantity;
class VolumeMesh;

class VolumeMeshColorQuantity : public VolumeMeshQuantity, public ColorQuantity<VolumeMeshColorQuantity> {
public:
  VolumeMeshColorQuantity(std::string name, VolumeMesh& mesh_, std::string definedOn,
                          const std::vector<glm::vec3>& colorValues);

  virtual void draw() override;
  virtual std::string niceName() override;
  virtual void refresh() override;

protected:
  // UI internals
  const std::string definedOn;
  std::shared_ptr<render::ShaderProgram> program;
  std::shared_ptr<render::ShaderProgram> sliceProgram;

  // Helpers
  virtual void createProgram() = 0;
};

// ========================================================
// ==========           Vertex Color             ==========
// ========================================================

class VolumeMeshVertexColorQuantity : public VolumeMeshColorQuantity {
public:
  VolumeMeshVertexColorQuantity(std::string name, VolumeMesh& mesh_, const std::vector<glm::vec3>& values_);

  virtual void createProgram() override;
  virtual std::shared_ptr<render::ShaderProgram> createSliceProgram() override;
  // void fillColorBuffers(render::ShaderProgram& p);
  void fillSliceColorBuffers(render::ShaderProgram& p);

  virtual void drawSlice(polyscope::SlicePlane* sp) override;

  void buildVertexInfoGUI(size_t vInd) override;
};

// ========================================================
// ==========             Cell Color             ==========
// ========================================================

class VolumeMeshCellColorQuantity : public VolumeMeshColorQuantity {
public:
  VolumeMeshCellColorQuantity(std::string name, VolumeMesh& mesh_, const std::vector<glm::vec3>& values_);

  // TODO add slice drawing similar to the vertex case above

  virtual void createProgram() override;
  // void fillColorBuffers(render::ShaderProgram& p);

  void buildCellInfoGUI(size_t cInd) override;
};

} // namespace polyscope
