// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/structure.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

Structure::Structure(std::string name_, std::string subtypeName)
    : name(name_), enabled(subtypeName + "#" + name + "#enabled", true) {}

Structure::~Structure(){};

Structure* Structure::setEnabled(bool newEnabled) {
  if (newEnabled == isEnabled()) return this;
  enabled = newEnabled;
  return this;
};

bool Structure::isEnabled() { return enabled.get(); };

void Structure::buildUI() {

  ImGui::PushID(name.c_str()); // ensure there are no conflicts with
                               // identically-named labels


  if (ImGui::TreeNode(name.c_str())) {

    bool currEnabled = isEnabled();
    ImGui::Checkbox("Enabled", &currEnabled);
    setEnabled(currEnabled);
    ImGui::SameLine();

    // Options popup
    if (ImGui::Button("Options")) {
      ImGui::OpenPopup("OptionsPopup");
    }
    if (ImGui::BeginPopup("OptionsPopup")) {

      // Transform
      if (ImGui::BeginMenu("Transform")) {
        if (ImGui::MenuItem("Center")) centerBoundingBox();
        if (ImGui::MenuItem("Unit Scale")) rescaleToUnit();
        if (ImGui::MenuItem("Reset")) resetTransform();
        ImGui::EndMenu();
      }

      // Do any structure-specific stuff here
      this->buildCustomOptionsUI();

      ImGui::EndPopup();
    }

    // Do any structure-specific stuff here
    this->buildCustomUI();

    // Build quantities list, in the common case of a QuantityStructure
    this->buildQuantitiesUI();

    ImGui::TreePop();
  }
  ImGui::PopID();
}


void Structure::buildQuantitiesUI() {}

void Structure::buildSharedStructureUI() {}

void Structure::buildCustomOptionsUI() {}

void Structure::resetTransform() {
  objectTransform = glm::mat4(1.0);
  updateStructureExtents();
}

void Structure::centerBoundingBox() {
  std::tuple<glm::vec3, glm::vec3> bbox = boundingBox();
  glm::vec3 center = (std::get<1>(bbox) + std::get<0>(bbox)) / 2.0f;
  glm::mat4x4 newTrans = glm::translate(glm::mat4x4(1.0), -glm::vec3(center.x, center.y, center.z));
  objectTransform = newTrans * objectTransform;
  updateStructureExtents();
}

void Structure::rescaleToUnit() {
  double currScale = lengthScale();
  float s = static_cast<float>(1.0 / currScale);
  glm::mat4x4 newTrans = glm::scale(glm::mat4x4(1.0), glm::vec3{s, s, s});
  objectTransform = newTrans * objectTransform;
  updateStructureExtents();
}

glm::mat4 Structure::getModelView() { return view::getCameraViewMatrix() * objectTransform; }

void Structure::setTransformUniforms(render::ShaderProgram& p) {
  glm::mat4 viewMat = getModelView();
  p.setUniform("u_modelView", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  p.setUniform("u_projMatrix", glm::value_ptr(projMat));
}

std::string Structure::uniquePrefix() { return typeName() + "#" + name + "#"; }

void Structure::remove() {
  removeStructure(typeName(), name);
}

} // namespace polyscope
