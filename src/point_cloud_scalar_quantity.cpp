// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/point_cloud_scalar_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {


PointCloudScalarQuantity::PointCloudScalarQuantity(std::string name, const std::vector<double>& values_,
                                                   PointCloud& pointCloud_, DataType dataType_)
    : PointCloudQuantity(name, pointCloud_, true), ScalarQuantity(*this, values_, dataType_)

{
  if (values_.size() != parent.points.size()) {
    polyscope::error("Point cloud scalar quantity " + name + " does not have same number of values (" +
                     std::to_string(values_.size()) + ") as point cloud size (" + std::to_string(parent.points.size()) +
                     ")");
  }
}

void PointCloudScalarQuantity::draw() {
  if (!isEnabled()) return;

  // Make the program if we don't have one already
  if (pointProgram == nullptr) {
    createPointProgram();
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


void PointCloudScalarQuantity::createPointProgram() {
  // Create the program to draw this quantity

  pointProgram = render::engine->requestShader("RAYCAST_SPHERE",
                                               parent.addPointCloudRules(addScalarRules({"SPHERE_PROPAGATE_VALUE"})));

  // Fill buffers
  parent.fillGeometryBuffers(*pointProgram);
  pointProgram->setAttribute("a_value", values);
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
  ImGui::Text("%g", values[ind]);
  ImGui::NextColumn();
}


std::string PointCloudScalarQuantity::niceName() { return name + " (scalar)"; }

} // namespace polyscope
