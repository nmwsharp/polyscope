// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/persistent_value.h"
#include "polyscope/render/engine.h"
#include "polyscope/utilities.h"
#include "polyscope/widget.h"

#include "ImGuizmo.h"

namespace polyscope {

// A visual widget with handles for translations/rotations

class TransformationGizmo : public Widget {

public:
  // == Constructors

  // Construct a gizmo.
  //
  // If T is null, this gizmo owns its transform matrix which can be accessed via set/getTransform().
  // If T is non-null, the gizmo will manipulate that external transform.  Optionally, a pointer can also be passed to a
  // PersistentValue<glm::mat4>, which will be updated as the transform is changed.
  //
  // Users creating additional gizmos should not call this, use addTransformationGizmo() instead.
  TransformationGizmo(std::string name, glm::mat4* T = nullptr, PersistentValue<glm::mat4>* Tpers = nullptr);


  // == Key members

  // a unique name
  const std::string name;

  // NOTE: this is only meaningful to call on use-created gizmos from addTransformationGizmo(),
  // it will have no effect on gizmos created other ways
  // After removing, the object is destructed
  void remove();

  // == Getters and setters

  glm::mat4 getTransform();
  void setTransform(glm::mat4 newT);

  bool getEnabled();
  void setEnabled(bool newVal);

  bool getAllowTranslation();
  void setAllowTranslation(bool newVal);

  bool getAllowRotation();
  void setAllowRotation(bool newVal);

  bool getAllowScaling();
  void setAllowScaling(bool newVal);

  // sadly this is not really supported by ImGuizmo
  // bool getUniformScaling();
  // void setUniformScaling(bool newVal);

  bool getInteractInLocalSpace();
  void setInteractInLocalSpace(bool newVal);

  // Size is relative, with 1.0 as the default size
  float getGizmoSize();
  void setGizmoSize(float newVal);

  // == Member functions

  std::string uniquePrefix() override;
  void draw() override;
  bool interact() override;
  void buildUI() override;
  void buildInlineTransformUI();
  void buildMenuItems();
  void markUpdated();

protected:
  // The main transform encoded by the gizmo
  // This can be either a reference to an external wrapped transform, or the internal storage below
  glm::mat4& Tref;

  // Optional, a persistent value defined elsewhere that goes with T, which
  // will be marked as updated when the gizmo changes the transform.
  PersistentValue<glm::mat4>* Tpers;

  // Local stoarage for a transformation.
  // This may or may not be used; based on the constructor args this class may wrap an external transform, or simply use
  // this one.
  glm::mat4 Towned = glm::mat4(1.0);

  // options
  PersistentValue<bool> enabled;
  PersistentValue<bool> allowTranslation;
  PersistentValue<bool> allowRotation;
  PersistentValue<bool> allowScaling;
  // PersistentValue<bool> uniformScaling; // not really supported by the ImGuizmo
  PersistentValue<bool> interactInLocalSpace;
  PersistentValue<bool> showUIWindow;
  PersistentValue<float> gizmoSize;

  // internal
  bool lastInteractResult = false;
};

// Create a user-defined transformation gizmo in the scene.
// By default, the gizmo maintains its own transformation matrix which
// can be accessed by by .getTransform(). Optionally, it can instead wrap
// and existin transform passed as transformToWrap.
TransformationGizmo* addTransformationGizmo(std::string name = "", glm::mat4* transformToWrap = nullptr);

// Get a user-created transformation gizmo by name
TransformationGizmo* getTransformationGizmo(std::string name);

// Remove a user-created transformation gizmo
void removeTransformationGizmo(TransformationGizmo* gizmo);
void removeTransformationGizmo(std::string name);
void removeAllTransformationGizmos();

} // namespace polyscope
