// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/transformation_gizmo.h"

#include "polyscope/polyscope.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "ImGuizmo.h"

#include <cmath>

namespace polyscope {

TransformationGizmo::TransformationGizmo(std::string name_, glm::mat4* externalTptr, PersistentValue<glm::mat4>* Tpers_)
    : name(name_), Tref(externalTptr ? *externalTptr : Towned), Tpers(Tpers_),
      // clang-format off
    enabled(name + "#enabled", false),
    allowTranslation(name + "#allowTranslation", true),
    allowRotation(name + "#allowRotation", true),
    allowScaling(name + "#allowScaling", false),
    // uniformScaling(name + "#uniformScaling", true),
    interactInLocalSpace(name + "#interactInLocalSpace", false),
    showUIWindow(name + "#showUIWindow", false),
    gizmoSize(name + "#gizmoSize", 1.0)
// clang-format on
{}


std::string TransformationGizmo::uniquePrefix() { return "#widget#TransformationGizmo#" + name; }

void TransformationGizmo::markUpdated() {
  if (Tpers != nullptr) {
    Tpers->manuallyChanged();
  }
  requestRedraw();
}

void TransformationGizmo::draw() {
  if (!enabled.get()) return;

  ImGuizmo::PushID(uniquePrefix().c_str());

  ImGuiIO& io = ImGui::GetIO();
  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

  // styling options
  ImGuizmo::SetGizmoSizeClipSpace(gizmoSize.get() * 0.08f * options::uiScale);

  ImGuizmo::Style& style = ImGuizmo::GetStyle();
  style.HatchedAxisLineThickness = 0.; // disable the hatching affect along the negative axis
  // setting these causes some weird clipping behaviors in the gizmo, let's not mess with it
  // style.TranslationLineThickness = 3.0f * options::uiScale;
  // style.TranslationLineArrowSize = 6.0f * options::uiScale;
  // style.RotationLineThickness = 2.0f * options::uiScale;
  // style.RotationOuterLineThickness = 3.0f * options::uiScale;
  // style.ScaleLineThickness = 3.0f * options::uiScale;

  int gizmoOpModeInt = 0;
  if (allowTranslation.get()) gizmoOpModeInt |= ImGuizmo::TRANSLATE;
  if (allowRotation.get()) gizmoOpModeInt |= ImGuizmo::ROTATE;
  if (allowScaling.get()) {
    gizmoOpModeInt |= ImGuizmo::SCALEU;
    // if(uniformScaling.get()) {
    //   gizmoOpModeInt |= ImGuizmo::SCALEU;
    // } else {
    //   gizmoOpModeInt |= ImGuizmo::SCALE;
    // }
  }
  ImGuizmo::OPERATION gizmoOpMode = static_cast<ImGuizmo::OPERATION>(gizmoOpModeInt);

  ImGuizmo::MODE gizmoCoordSpace = interactInLocalSpace.get() ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

  glm::mat4 cameraView = view::getCameraViewMatrix();
  glm::mat4 cameraProjection = view::getCameraPerspectiveMatrix();
  ImGuizmo::SetOrthographic(view::getProjectionMode() == ProjectionMode::Orthographic);

  // draw/process the actual widget
  bool lastInteractResult = ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                                                 gizmoOpMode, gizmoCoordSpace, glm::value_ptr(Tref));

  if (lastInteractResult) {
    markUpdated();
  }

  ImGuizmo::PopID();
}


void TransformationGizmo::buildMenuItems() {
  if (ImGui::MenuItem("Show Gizmo", NULL, &enabled.get())) enabled.manuallyChanged();
  ImGui::MenuItem("Show Transform Window", NULL, &showUIWindow.get());
  ImGui::MenuItem("Allow Translation", NULL, &allowTranslation.get());
  ImGui::MenuItem("Allow Rotation", NULL, &allowRotation.get());
  ImGui::MenuItem("Allow Scaling", NULL, &allowScaling.get());
  // ImGui::MenuItem("Uniform Scaling", NULL, &uniformScaling.get());
}

bool TransformationGizmo::interact() {
  if (!enabled.get()) return false;

  // it might might be slightly more correct to invoke the ImGuizmo::Manipulate() or do a ImGuizmo::IsUsing() check
  // here. But it seems mostly fine to just re-return the last value, and that is much simpler. Let's do this until we
  // have a reason to change it.

  return lastInteractResult;
}


void TransformationGizmo::buildUI() {
  if (!showUIWindow.get()) return;

  bool showUIWindowBefore = showUIWindow.get();
  if (ImGui::Begin(name.c_str(), &showUIWindow.get())) {
    buildInlineTransformUI();
  }
  ImGui::End();

  if (showUIWindowBefore != showUIWindow.get()) {
    showUIWindow.manuallyChanged();
  }
}

void TransformationGizmo::buildInlineTransformUI() {

  ImGui::PushID(uniquePrefix().c_str());

  ImGui::Checkbox("Gizmo Enabled", &enabled.get());

  ImGui::Checkbox("Translation", &allowTranslation.get());
  ImGui::SameLine();
  ImGui::Checkbox("Rotation", &allowRotation.get());
  ImGui::SameLine();
  ImGui::Checkbox("Scaling", &allowScaling.get());

  // if (allowScaling.get()) {
  //   ImGui::Checkbox("Uniform Scaling", &uniformScaling.get());
  // }

  static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
  static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
  if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
    mCurrentGizmoOperation = ImGuizmo::ROTATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE)) mCurrentGizmoOperation = ImGuizmo::SCALE;
  float matrixTranslation[3], matrixRotation[3], matrixScale[3];
  ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(Tref), matrixTranslation, matrixRotation, matrixScale);
  ImGui::InputFloat3("Tr", matrixTranslation);
  ImGui::InputFloat3("Rt", matrixRotation);
  ImGui::InputFloat3("Sc", matrixScale);
  ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, glm::value_ptr(Tref));

  // TODO these don't seem to be doing anything?
  if (ImGui::RadioButton("Local", interactInLocalSpace.get())) interactInLocalSpace = true;
  ImGui::SameLine();
  if (ImGui::RadioButton("World", !interactInLocalSpace.get())) interactInLocalSpace = false;

  ImGui::PopID();
}

glm::mat4 TransformationGizmo::getTransform() { return Tref; }
void TransformationGizmo::setTransform(glm::mat4 newT) {
  Tref = newT;
  markUpdated();
};

bool TransformationGizmo::getEnabled() { return enabled.get(); }
void TransformationGizmo::setEnabled(bool newVal) { enabled = newVal; }

bool TransformationGizmo::getAllowTranslation() { return allowTranslation.get(); }
void TransformationGizmo::setAllowTranslation(bool newVal) { allowTranslation = newVal; }

bool TransformationGizmo::getAllowRotation() { return allowRotation.get(); }
void TransformationGizmo::setAllowRotation(bool newVal) { allowRotation = newVal; }

bool TransformationGizmo::getAllowScaling() { return allowScaling.get(); }
void TransformationGizmo::setAllowScaling(bool newVal) { allowScaling = newVal; }

bool TransformationGizmo::getInteractInLocalSpace() { return interactInLocalSpace.get(); }
void TransformationGizmo::setInteractInLocalSpace(bool newVal) { interactInLocalSpace = newVal; }


TransformationGizmo* addTransformationGizmo(std::string name, glm::mat4* transformToWrap) {

  std::vector<std::unique_ptr<TransformationGizmo>>& gizmoList = state::globalContext.createdTransformationGizmos;

  // If auto-naming, assign a unique name
  if (name == "") {
    // assign a name like transformation_gizmo_7
    int iName = 0;
    bool taken = true;
    name = "transformation_gizmo_" + std::to_string(iName);
    while (taken) {
      taken = false;
      for (auto& gizmo : gizmoList) {
        if (gizmo->name == name) {
          taken = true;
          break;
        }
      }
      if (taken) {
        iName++;
        name = "transformation_gizmo_" + std::to_string(iName);
      }
    }
  }

  // Check that the name is unique
  for (auto& gizmo : gizmoList) {
    if (gizmo->name == name) {
      error("Transformation Gizmo already exists with name " + name);
      return nullptr;
    }
  }

  // create the gizmo
  gizmoList.emplace_back(std::unique_ptr<TransformationGizmo>(new TransformationGizmo(name, transformToWrap)));

  TransformationGizmo* newGizmo = gizmoList.back().get();
  newGizmo->setEnabled(true);

  return newGizmo;
}

void removeTransformationGizmo(TransformationGizmo* gizmo) {
  std::vector<std::unique_ptr<TransformationGizmo>>& gizmoList = state::globalContext.createdTransformationGizmos;

  // erase-remove the gizmo matching the pointer (assuming there is one)
  gizmoList.erase(std::remove_if(gizmoList.begin(), gizmoList.end(),
                                 [gizmo](const std::unique_ptr<TransformationGizmo>& g) { return g.get() == gizmo; }),
                  gizmoList.end());
}

void removeTransformationGizmo(std::string name) {
  std::vector<std::unique_ptr<TransformationGizmo>>& gizmoList = state::globalContext.createdTransformationGizmos;

  // find the gizmo with the matching name and remove it
  for (auto& gizmo : gizmoList) {
    if (gizmo->name == name) {
      removeTransformationGizmo(gizmo.get());
      return;
    }
  }
}

void removeAllTransformationGizmos() {
  std::vector<std::unique_ptr<TransformationGizmo>>& gizmoList = state::globalContext.createdTransformationGizmos;
  gizmoList.clear();
}

} // namespace polyscope
