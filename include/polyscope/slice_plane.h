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

  // Name and postfix should both be unique
  SlicePlane(std::string name);
  ~SlicePlane();

  void deregister(); // This is essentially a destructor, but we invoke it manually to avoid dealing with destructor
                     // order and static variables / singletons. Call it when deleteing a slice plane, but not in the
                     // midst of program exit. ONEDAY: handle global lists & registration better.

  // No copy constructor/assignment
  SlicePlane(const SlicePlane&) = delete;
  SlicePlane& operator=(const SlicePlane&) = delete;

  void buildGUI();
  void draw();

  void setSceneObjectUniforms(render::ShaderProgram& p,
                              bool alwaysPass = false); // if alwaysPass, fake values are given so the plane does
                                                        // nothing (regardless of this plane's active setting)

  const std::string name;
  const std::string postfix;

  // == Some getters and setters

  bool getActive();
  void setActive(bool newVal);

  bool getDrawPlane();
  void setDrawPlane(bool newVal);

  glm::mat4 getTransform();
  void setTransform(glm::mat4 newTransform);

protected:
  // = State
  PersistentValue<bool> active;    // is it actually slicing?
  PersistentValue<bool> drawPlane; // do we draw the plane onscreen?
  PersistentValue<glm::mat4> objectTransform;
  PersistentValue<glm::vec3> color;
  PersistentValue<float> transparency;

  // Widget that wraps the transform
  TransformationGizmo transformGizmo;

  std::shared_ptr<render::ShaderProgram> planeProgram;

  // Helpers
  void prepare();
  glm::vec3 getCenter();
  glm::vec3 getNormal();
};

SlicePlane* addSceneSlicePlane();
void removeLastSceneSlicePlane();
void buildSlicePlaneGUI();

} // namespace polyscope
