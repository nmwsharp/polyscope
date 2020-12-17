// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/surface_color_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

SurfaceColorQuantity::SurfaceColorQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn_)
    : SurfaceMeshQuantity(name, mesh_, true), definedOn(definedOn_) {}

void SurfaceColorQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*program);
  parent.setStructureUniforms(*program);

  program->draw();
}

// ========================================================
// ==========           Vertex Color            ==========
// ========================================================

SurfaceVertexColorQuantity::SurfaceVertexColorQuantity(std::string name, std::vector<glm::vec3> values_,
                                                       SurfaceMesh& mesh_)
    : SurfaceColorQuantity(name, mesh_, "vertex"), values(std::move(values_))

{}

void SurfaceVertexColorQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addStructureRules({"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceVertexColorQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<glm::vec3> colorval;
  colorval.reserve(3 * parent.nFacesTriangulation());

  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();

    // implicitly triangulate from root
    size_t vRoot = face[0];
    for (size_t j = 1; (j + 1) < D; j++) {
      size_t vB = face[j];
      size_t vC = face[(j + 1) % D];

      colorval.push_back(values[vRoot]);
      colorval.push_back(values[vB]);
      colorval.push_back(values[vC]);
    }
  }

  // Store data in buffers
  p.setAttribute("a_color", colorval);
}

void SurfaceVertexColorQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = values[vInd];
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

SurfaceFaceColorQuantity::SurfaceFaceColorQuantity(std::string name, std::vector<glm::vec3> values_, SurfaceMesh& mesh_)
    : SurfaceColorQuantity(name, mesh_, "face"), values(std::move(values_))

{}

void SurfaceFaceColorQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addStructureRules({"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SurfaceFaceColorQuantity::fillColorBuffers(render::ShaderProgram& p) {
  std::vector<glm::vec3> colorval;
  colorval.reserve(3 * parent.nFacesTriangulation());

  for (size_t iF = 0; iF < parent.nFaces(); iF++) {
    auto& face = parent.faces[iF];
    size_t D = face.size();
    size_t triDegree = std::max(0, static_cast<int>(D) - 2);
    for (size_t j = 0; j < 3 * triDegree; j++) {
      colorval.push_back(values[iF]);
    }
  }


  // Store data in buffers
  p.setAttribute("a_color", colorval);
}

void SurfaceFaceColorQuantity::buildFaceInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = values[fInd];
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << values[fInd];
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}

} // namespace polyscope
