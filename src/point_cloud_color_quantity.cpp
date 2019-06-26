#include "polyscope/point_cloud_color_quantity.h"

#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {


PointCloudColorQuantity::PointCloudColorQuantity(std::string name, const std::vector<glm::vec3>& values_,
                                                 PointCloud& pointCloud_)
    : PointCloudQuantityThatDrawsPoints(name, pointCloud_)

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
  if (!enabled) return;

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
  pointProgram.reset(new gl::GLProgram(&SPHERE_COLOR_VERT_SHADER, &SPHERE_COLOR_BILLBOARD_GEOM_SHADER,
                                       &SPHERE_COLOR_BILLBOARD_FRAG_SHADER, gl::DrawMode::Points));

  // Fill buffers
  pointProgram->setAttribute("a_position", parent.points);
  pointProgram->setAttribute("a_color", values);

  setMaterialForProgram(*pointProgram, "wax");
}


void PointCloudColorQuantity::buildPickUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g, %g, %g", values[ind].x, values[ind].y, values[ind].z);
  ImGui::NextColumn();
}

} // namespace polyscope
