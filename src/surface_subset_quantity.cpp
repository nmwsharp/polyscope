// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_subset_quantity.h"

#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/cylinder_shaders.h"
#include "polyscope/polyscope.h"
#include "polyscope/utilities.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceEdgeSubsetQuantity::SurfaceEdgeSubsetQuantity(std::string name, std::vector<char> edgeSubset_,
                                                     SurfaceMesh* mesh_)
    : SurfaceQuantity(name, mesh_), edgeSubset(std::move(edgeSubset_))

{

  // Create the program
  safeDelete(program);
  program = new gl::GLProgram(&PASSTHRU_CYLINDER_VERT_SHADER, &CYLINDER_GEOM_SHADER, &SHINY_CYLINDER_FRAG_SHADER,
                              gl::DrawMode::Points);

  // Fill buffers
  count = 0;
  std::vector<glm::vec3> pTail, pTip;
  for (size_t iE = 0; iE < parent->nEdges(); iE++) {
    if (edgeSubset[iE]) {
      HalfedgeMesh::Edge& edge = parent->mesh.edges[iE];
      pTail.push_back(edge.halfedge().vertex().position());
      pTip.push_back(edge.halfedge().twin().vertex().position());
      count++;
    }
  }

  program->setAttribute("a_position_tail", pTail);
  program->setAttribute("a_position_tip", pTip);

  setMaterialForProgram(program, "wax");

  // initialize the color to something nice
  color = parent->colorManager.getNextSubColor(name);
}

SurfaceEdgeSubsetQuantity::~SurfaceEdgeSubsetQuantity() { safeDelete(program); }

void SurfaceEdgeSubsetQuantity::draw() {

  if (enabled) {

    // Set uniforms
    glm::mat4 viewMat = parent->getModelView();
    program->setUniform("u_modelView", glm::value_ptr(viewMat));

    glm::mat4 projMat = view::getCameraPerspectiveMatrix();
    program->setUniform("u_projMatrix", glm::value_ptr(projMat));

    program->setUniform("u_radius", radius * state::lengthScale);
    program->setUniform("u_color", color);


    program->draw();
  }
}

void SurfaceEdgeSubsetQuantity::drawUI() {

  if (ImGui::TreeNode((name + " (edge subset)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::SameLine();
    ImGui::ColorEdit3("Color", (float*)&color, ImGuiColorEditFlags_NoInputs);
    ImGui::Text("Count: %d", count);
    ImGui::SliderFloat("Radius", &radius, 0.0, .1, "%.5f", 3.);
    ImGui::TreePop();
  }
}

void SurfaceEdgeSubsetQuantity::buildEdgeInfoGUI(size_t eInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  if (edgeSubset[eInd]) {
    ImGui::TextUnformatted("yes");
  } else {
    ImGui::TextUnformatted("no");
  }
  ImGui::NextColumn();
}

} // namespace polyscope
