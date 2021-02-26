#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/render/engine.h"
#include "polyscope/transformation_gizmo.h"
#include "polyscope/utilities.h"
#include "polyscope/widget.h"

namespace polyscope {

class SlicePlane {

public:
  // == Constructors
  SlicePlane(std::string name, std::string postfix);
  ~SlicePlane();

  // No copy constructor/assignment
  SlicePlane(const SlicePlane&) = delete;
  SlicePlane& operator=(const SlicePlane&) = delete;

  void buildGUI();
  void draw();

  void setSceneObjectUniforms(render::ShaderProgram& p);

protected:
  const std::string name;
  const std::string postfix;

  // = State
  PersistentValue<bool> enabled;
  PersistentValue<bool> show;
  PersistentValue<glm::mat4> objectTransform;
  PersistentValue<float> transparency;

  // Widget that wraps the transform
  TransformationGizmo transformGizmo;
  
  std::shared_ptr<render::ShaderProgram> planeProgram;

  // Helpers
  void prepare();
  glm::vec3 getCenter();
  glm::vec3 getNormal();
};

void buildSlicePlaneGUI();
SlicePlane* addSlicePlane();

} // namespace polyscope
