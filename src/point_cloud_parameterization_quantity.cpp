// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/point_cloud_parameterization_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

namespace polyscope {

PointCloudParameterizationQuantity::PointCloudParameterizationQuantity(std::string name, PointCloud& cloud_,
                                                                       const std::vector<glm::vec2>& coords_,
                                                                       ParamCoordsType type_, ParamVizStyle style_)
    : PointCloudQuantity(name, cloud_, true), ParameterizationQuantity(*this, coords_, type_, style_) {}


void PointCloudParameterizationQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  setParameterizationUniforms(*program);
  parent.setStructureUniforms(*program);
  parent.setPointCloudUniforms(*program);

  program->draw();
}

void PointCloudParameterizationQuantity::createProgram() {

  // Create the program to draw this quantity
  program =
      render::engine->requestShader(parent.getShaderNameForRenderMode(),
                                    parent.addPointCloudRules(addParameterizationRules({"SPHERE_PROPAGATE_VALUE2"})));

  // Fill buffers
  fillCoordBuffers(*program);
  fillParameterizationBuffers(*program);
  parent.setPointProgramGeometryAttributes(*program);

  render::engine->setMaterial(*program, parent.getMaterial());
}

void PointCloudParameterizationQuantity::fillCoordBuffers(render::ShaderProgram& p) {
  p.setAttribute("a_value2", coords.getRenderAttributeBuffer());
}

void PointCloudParameterizationQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    buildParameterizationOptionsUI();

    ImGui::EndPopup();
  }

  buildParameterizationUI();
}

void PointCloudParameterizationQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string PointCloudParameterizationQuantity::niceName() { return name + " (parameterization)"; }

void PointCloudParameterizationQuantity::buildPickUI(size_t ind) {

  glm::vec2 coord = coords.getValue(ind);

  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("<%g,%g>", coord.x, coord.y);
  ImGui::NextColumn();
}


} // namespace polyscope
