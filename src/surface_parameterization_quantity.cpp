// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/surface_parameterization_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

namespace polyscope {

// ==============================================================
// ================  Base Parameterization  =====================
// ==============================================================

SurfaceParameterizationQuantity::SurfaceParameterizationQuantity(std::string name, SurfaceMesh& mesh_,
                                                                 const std::vector<glm::vec2>& coords_,
                                                                 ParamCoordsType type_, ParamVizStyle style_)
    : SurfaceMeshQuantity(name, mesh_, true), ParameterizationQuantity(*this, coords_, type_, style_) {}

void SurfaceParameterizationQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  setParameterizationUniforms(*program);
  parent.setStructureUniforms(*program);
  parent.setSurfaceMeshUniforms(*program);

  program->draw();
}

void SurfaceParameterizationQuantity::createProgram() {

  // Create the program to draw this quantity
  program = render::engine->requestShader(
      "MESH", parent.addSurfaceMeshRules(addParameterizationRules({"MESH_PROPAGATE_VALUE2"})));

  // Fill buffers
  fillCoordBuffers(*program);
  fillParameterizationBuffers(*program);
  parent.setMeshGeometryAttributes(*program);

  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceParameterizationQuantity::buildCustomUI() {
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


void SurfaceParameterizationQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

// ==============================================================
// ===============  Corner Parameterization  ====================
// ==============================================================


SurfaceCornerParameterizationQuantity::SurfaceCornerParameterizationQuantity(std::string name, SurfaceMesh& mesh_,
                                                                             const std::vector<glm::vec2>& coords_,
                                                                             ParamCoordsType type_,
                                                                             ParamVizStyle style_)
    : SurfaceParameterizationQuantity(name, mesh_, coords_, type_, style_) {}

std::string SurfaceCornerParameterizationQuantity::niceName() { return name + " (corner parameterization)"; }


void SurfaceCornerParameterizationQuantity::fillCoordBuffers(render::ShaderProgram& p) {
  p.setAttribute("a_value2", coords.getIndexedRenderAttributeBuffer(parent.triangleCornerInds));
}

void SurfaceCornerParameterizationQuantity::buildCornerInfoGUI(size_t cInd) {

  glm::vec2 coord = coords.getValue(cInd);

  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("<%g,%g>", coord.x, coord.y);
  ImGui::NextColumn();
}

// ==============================================================
// ===============  Vertex Parameterization  ====================
// ==============================================================


SurfaceVertexParameterizationQuantity::SurfaceVertexParameterizationQuantity(std::string name, SurfaceMesh& mesh_,
                                                                             const std::vector<glm::vec2>& coords_,
                                                                             ParamCoordsType type_,
                                                                             ParamVizStyle style_)
    : SurfaceParameterizationQuantity(name, mesh_, coords_, type_, style_) {}

std::string SurfaceVertexParameterizationQuantity::niceName() { return name + " (vertex parameterization)"; }

void SurfaceVertexParameterizationQuantity::fillCoordBuffers(render::ShaderProgram& p) {
  p.setAttribute("a_value2", coords.getIndexedRenderAttributeBuffer(parent.triangleVertexInds));
}

void SurfaceVertexParameterizationQuantity::buildVertexInfoGUI(size_t vInd) {

  glm::vec2 coord = coords.getValue(vInd);

  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("<%g,%g>", coord.x, coord.y);
  ImGui::NextColumn();
}


} // namespace polyscope
