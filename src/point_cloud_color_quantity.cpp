#include "polyscope/point_cloud_color_quantity.h"

#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {


PointCloudColorQuantity::PointCloudColorQuantity(std::string name, const std::vector<Vector3>& values_,
                                                 PointCloud* pointCloud_)
    : PointCloudQuantityThatDrawsPoints(name, pointCloud_)

{

  if (values_.size() != parent->points.size()) {
    polyscope::error("Point cloud color quantity " + name + " does not have same number of values (" +
                     std::to_string(values_.size()) + ") as point cloud size (" +
                     std::to_string(parent->points.size()) + ")");
  }

  // Copy the raw data
  values = values_;
}


void PointCloudColorQuantity::drawUI() {
  bool enabledBefore = enabled;
  if (ImGui::TreeNode((name + " (color)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);

    ImGui::TreePop();
  }

  // Enforce exclusivity of enabled surface quantities
  if (!enabledBefore && enabled) {
    parent->setActiveQuantity(this);
  }
  if (enabledBefore && !enabled) {
    parent->clearActiveQuantity();
  }
}


gl::GLProgram* PointCloudColorQuantity::createProgram() {
  // Create the program to draw this quantity

  gl::GLProgram* program;
  if (parent->requestsBillboardSpheres()) {
    program = new gl::GLProgram(&SPHERE_COLOR_VERT_SHADER, &SPHERE_COLOR_BILLBOARD_GEOM_SHADER,
                                &SPHERE_COLOR_BILLBOARD_FRAG_SHADER, gl::DrawMode::Points);
  } else {
    program = new gl::GLProgram(&SPHERE_COLOR_VERT_SHADER, &SPHERE_COLOR_GEOM_SHADER, &SPHERE_COLOR_FRAG_SHADER,
                                gl::DrawMode::Points);
  }

  // Fill color buffers
  fillColorBuffers(program);

  return program;
}

bool PointCloudColorQuantity::wantsBillboardUniforms() { return parent->requestsBillboardSpheres(); }

void PointCloudColorQuantity::fillColorBuffers(gl::GLProgram* p) {
  // Store data in buffers
  p->setAttribute("a_color", values);
}

void PointCloudColorQuantity::buildInfoGUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g, %g, %g", values[ind].x, values[ind].y, values[ind].z);
  ImGui::NextColumn();
}

} // namespace polyscope
