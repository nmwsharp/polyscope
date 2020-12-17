// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/surface_mesh.h"

#include <tuple>

namespace polyscope {

class SurfaceGraphQuantity : public SurfaceMeshQuantity {
public:
  SurfaceGraphQuantity(std::string name, std::vector<glm::vec3> nodes, std::vector<std::array<size_t, 2>> edges,
                       SurfaceMesh& mesh_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;
  virtual void refresh() override;

  std::vector<glm::vec3> nodes;
  std::vector<std::array<size_t, 2>> edges;

  // == Option setters and getters
  SurfaceGraphQuantity* setRadius(double newVal, bool isRelative = true);
  double getRadius();

  SurfaceGraphQuantity* setColor(glm::vec3 newColor);
  glm::vec3 getColor();

private:
  // === Appearance options
  PersistentValue<ScaledValue<float>> radius;
  PersistentValue<glm::vec3> color;

  std::shared_ptr<render::ShaderProgram> pointProgram;
  std::shared_ptr<render::ShaderProgram> lineProgram;

  void createPrograms();
  void setUniforms();
};


} // namespace polyscope
