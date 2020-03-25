// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/point_cloud_color_quantity.h"

#include "polyscope/render/shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {


PointCloudColorQuantity::PointCloudColorQuantity(std::string name, const std::vector<glm::vec3>& values_,
                                                 PointCloud& pointCloud_)
    : PointCloudQuantity(name, pointCloud_, true)

{

  if (values_.size() != parent.points.size()) {
    polyscope::error("Point cloud color quantity " + name + " does not have same number of values (" +
                     std::to_string(values_.size()) + ") as point cloud size (" + std::to_string(parent.points.size()) +
                     ")");
  }

  // Copy the raw data
  values = values_;
}

void PointCloudColorQuantity::draw() {
  if (!isEnabled()) return;

  // Make the program if we don't have one already
  if (pointProgram == nullptr) {
    createPointProgram();
  }

  parent.setTransformUniforms(*pointProgram);
  parent.setPointCloudUniforms(*pointProgram);

  pointProgram->draw();
}

std::string PointCloudColorQuantity::niceName() { return name + " (color)"; }

void PointCloudColorQuantity::createPointProgram() {
  // Create the program to draw this quantity
  pointProgram = render::engine->generateShaderProgram({render::SPHERE_COLOR_VERT_SHADER,
                                                        render::SPHERE_COLOR_BILLBOARD_GEOM_SHADER,
                                                        render::SPHERE_COLOR_BILLBOARD_FRAG_SHADER},
                                                       DrawMode::Points);

  // Fill buffers
  pointProgram->setAttribute("a_position", parent.points);
  pointProgram->setAttribute("a_color", values);

  render::engine->setMaterial(*pointProgram, parent.getMaterial());
}

void PointCloudColorQuantity::geometryChanged() { pointProgram.reset(); }


void PointCloudColorQuantity::buildPickUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = values[ind];
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::string colorStr = to_string_short(tempColor);
  ImGui::TextUnformatted(colorStr.c_str());
  ImGui::NextColumn();
}

} // namespace polyscope
