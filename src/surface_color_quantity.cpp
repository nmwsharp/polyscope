// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/surface_color_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

SurfaceColorQuantity::SurfaceColorQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn_,
                                           const std::vector<glm::vec3>& colorValues_)
    : SurfaceMeshQuantity(name, mesh_, true), ColorQuantity(*this, colorValues_), definedOn(definedOn_) {}

void SurfaceColorQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*program);
  parent.setSurfaceMeshUniforms(*program);

  program->draw();
}

// ========================================================
// ==========           Vertex Color            ==========
// ========================================================

SurfaceVertexColorQuantity::SurfaceVertexColorQuantity(std::string name, SurfaceMesh& mesh_,
                                                       std::vector<glm::vec3> colorValues_)
    : SurfaceColorQuantity(name, mesh_, "vertex", colorValues_)

{}

void SurfaceVertexColorQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addSurfaceMeshRules({"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}));

  parent.setMeshGeometryAttributes(*program);
  program->setAttribute("a_color", colors.getIndexedRenderAttributeBuffer(parent.triangleVertexInds));
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceVertexColorQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = colors.getValue(vInd);
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::string colorStr = to_string_short(tempColor);
  ImGui::TextUnformatted(colorStr.c_str());
  ImGui::NextColumn();
}

std::string SurfaceColorQuantity::niceName() { return name + " (" + definedOn + " color)"; }

void SurfaceColorQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

// ========================================================
// ==========            Face Color              ==========
// ========================================================

SurfaceFaceColorQuantity::SurfaceFaceColorQuantity(std::string name, SurfaceMesh& mesh_,
                                                   std::vector<glm::vec3> colorValues_)
    : SurfaceColorQuantity(name, mesh_, "face", colorValues_)

{}

void SurfaceFaceColorQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addSurfaceMeshRules({"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}));

  parent.setMeshGeometryAttributes(*program);
  program->setAttribute("a_color", colors.getIndexedRenderAttributeBuffer(parent.triangleFaceInds));
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceFaceColorQuantity::buildFaceInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = colors.getValue(fInd);
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << tempColor;
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}

} // namespace polyscope
