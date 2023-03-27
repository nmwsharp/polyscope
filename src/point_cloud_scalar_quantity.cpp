// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/point_cloud_scalar_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {


PointCloudScalarQuantity::PointCloudScalarQuantity(std::string name, const std::vector<double>& values_,
                                                   PointCloud& pointCloud_, DataType dataType_)
    : PointCloudQuantity(name, pointCloud_, true), ScalarQuantity(*this, values_, dataType_) {}

void PointCloudScalarQuantity::draw() {
  if (!isEnabled()) return;

  // Make the program if we don't have one already
  if (pointProgram == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*pointProgram);
  parent.setPointCloudUniforms(*pointProgram);
  setScalarUniforms(*pointProgram);

  pointProgram->draw();
}


void PointCloudScalarQuantity::buildCustomUI() {
  ImGui::SameLine();

  // == Options popup
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {

    buildScalarOptionsUI();

    ImGui::EndPopup();
  }

  buildScalarUI();
}


void PointCloudScalarQuantity::createProgram() {

  // Create the program to draw this quantity
  // clang-format off
  pointProgram = render::engine->requestShader(
      parent.getShaderNameForRenderMode(), 
      parent.addPointCloudRules(addScalarRules({"SPHERE_PROPAGATE_VALUE"}))
  );
  // clang-format on

  parent.setPointProgramGeometryAttributes(*pointProgram);
  pointProgram->setAttribute("a_value", values.getRenderAttributeBuffer());

  // Fill buffers
  pointProgram->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*pointProgram, parent.getMaterial());
}


void PointCloudScalarQuantity::refresh() {
  pointProgram.reset();
  Quantity::refresh();
}

void PointCloudScalarQuantity::buildPickUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(ind));
  ImGui::NextColumn();
}


std::string PointCloudScalarQuantity::niceName() { return name + " (scalar)"; }

} // namespace polyscope
