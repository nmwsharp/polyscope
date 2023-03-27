// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/point_cloud_color_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {


PointCloudColorQuantity::PointCloudColorQuantity(std::string name, const std::vector<glm::vec3>& values_,
                                                 PointCloud& pointCloud_)
    : PointCloudQuantity(name, pointCloud_, true), ColorQuantity(*this, values_) {}

void PointCloudColorQuantity::draw() {
  if (!isEnabled()) return;

  // Make the program if we don't have one already
  if (pointProgram == nullptr) {
    createPointProgram();
  }

  parent.setStructureUniforms(*pointProgram);
  parent.setPointCloudUniforms(*pointProgram);
  setColorUniforms(*pointProgram);

  pointProgram->draw();
}

std::string PointCloudColorQuantity::niceName() { return name + " (color)"; }

void PointCloudColorQuantity::createPointProgram() {

  // Create the program to draw this quantity
  // clang-format off
  pointProgram = render::engine->requestShader(
      parent.getShaderNameForRenderMode(), 
      parent.addPointCloudRules({"SPHERE_PROPAGATE_COLOR", "SHADE_COLOR"})
  );
  // clang-format on

  parent.setPointProgramGeometryAttributes(*pointProgram);
  pointProgram->setAttribute("a_color", colors.getRenderAttributeBuffer());

  // Fill buffers
  render::engine->setMaterial(*pointProgram, parent.getMaterial());
}


void PointCloudColorQuantity::refresh() {
  pointProgram.reset();
  Quantity::refresh();
}


void PointCloudColorQuantity::buildPickUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 color = colors.getValue(ind);
  ImGui::ColorEdit3("", &color[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::string colorStr = to_string_short(color);
  ImGui::TextUnformatted(colorStr.c_str());
  ImGui::NextColumn();
}

} // namespace polyscope
