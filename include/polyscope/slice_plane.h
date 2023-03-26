// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/render/engine.h"
#include "polyscope/render/managed_buffer.h"
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

  // No copy constructor/assignment
  SlicePlane(const SlicePlane&) = delete;
  SlicePlane& operator=(const SlicePlane&) = delete;

  void buildGUI();
  void draw();
  void drawGeometry();
  void resetVolumeSliceProgram();
  void ensureVolumeInspectValid();

  void setSceneObjectUniforms(render::ShaderProgram& p,
                              bool alwaysPass = false); // if alwaysPass, fake values are given so the plane does
                                                        // nothing (regardless of this plane's active setting)
  void setSliceGeomUniforms(render::ShaderProgram& p);

  const std::string name;
  const std::string postfix;
  std::string uniquePrefix();

  // Set the position and orientation of the plane
  // planePosition is any 3D position which the plane touches (the center of the plane)
  // planeNormal is a vector giving the normal direction of the plane, objects
  // in this negative side of the plane will be culled
  void setPose(glm::vec3 planePosition, glm::vec3 planeNormal);

  // == Some getters and setters

  bool getActive();
  void setActive(bool newVal);

  bool getDrawPlane();
  void setDrawPlane(bool newVal);

  bool getDrawWidget();
  void setDrawWidget(bool newVal);

  glm::mat4 getTransform();
  void setTransform(glm::mat4 newTransform);

  void setColor(glm::vec3 newVal);
  glm::vec3 getColor();

  void setGridLineColor(glm::vec3 newVal);
  glm::vec3 getGridLineColor();

  void setTransparency(double newVal);
  double getTransparency();

  void setVolumeMeshToInspect(std::string meshName);
  std::string getVolumeMeshToInspect();

protected:
  // = State
  PersistentValue<bool> active;     // is it actually slicing?
  PersistentValue<bool> drawPlane;  // do we draw the plane onscreen?
  PersistentValue<bool> drawWidget; // do we draw the widget onscreen?
  PersistentValue<glm::mat4> objectTransform;
  PersistentValue<glm::vec3> color;
  PersistentValue<glm::vec3> gridLineColor;
  PersistentValue<float> transparency;

  // DON'T make these persistent, because it is unintitive to re-add a scene slice plane and have it immediately start
  // slicing
  bool shouldInspectMesh;
  std::string inspectedMeshName;

  std::shared_ptr<render::ShaderProgram> volumeInspectProgram;

  // Widget that wraps the transform
  TransformationGizmo transformGizmo;

  // these are optionally filled when slice-visualizing into a volume mesh
  std::array<std::vector<uint32_t>, 4> sliceBufferDataArr;
  std::array<render::ManagedBuffer<uint32_t>, 4> sliceBufferArr;

  std::shared_ptr<render::ShaderProgram> planeProgram;

  // Helpers
  void setSliceAttributes(render::ShaderProgram& p);
  void createVolumeSliceProgram();
  void prepare();
  glm::vec3 getCenter();
  glm::vec3 getNormal();
  void updateWidgetEnabled();
};

SlicePlane* addSceneSlicePlane(bool initiallyVisible = false);
void removeLastSceneSlicePlane();
void buildSlicePlaneGUI();

// flag to open the slice plane menu after adding a slice plane
extern bool openSlicePlaneMenu;

} // namespace polyscope
