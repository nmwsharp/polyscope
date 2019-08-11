// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/structure.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

Structure::Structure(std::string name_) : name(name_) {}

Structure::~Structure(){};

void Structure::setEnabled(bool newEnabled) {
  if (newEnabled == enabled) return;
  enabled = newEnabled;
};

bool Structure::isEnabled() { return enabled; };

void Structure::buildUI() {

  ImGui::PushID(name.c_str()); // ensure there are no conflicts with
                               // identically-named labels


  if (ImGui::TreeNode(name.c_str())) {

    bool currEnabled = enabled;
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
  objectTransform = objectTransform * newTrans;
  updateStructureExtents();
}

glm::mat4 Structure::getModelView() { return view::getCameraViewMatrix() * objectTransform; }

void Structure::setTransformUniforms(gl::GLProgram& p) {
  glm::mat4 viewMat = getModelView();
  p.setUniform("u_modelView", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  p.setUniform("u_projMatrix", glm::value_ptr(projMat));
}

} // namespace polyscope
