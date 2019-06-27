#include "polyscope/surface_color_quantity.h"

#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/surface_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

SurfaceColorQuantity::SurfaceColorQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn_)
    : SurfaceMeshQuantity(name, mesh_, true), definedOn(definedOn_) {}

void SurfaceColorQuantity::draw() {
  if (!enabled) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setTransformUniforms(*program);
  parent.setMeshUniforms(*program);

  program->draw();
}

// ========================================================
// ==========           Vertex Color            ==========
// ========================================================

SurfaceColorVertexQuantity::SurfaceColorVertexQuantity(std::string name, std::vector<Color3f> values_,
                                                       SurfaceMesh& mesh_)
    : SurfaceColorQuantity(name, mesh_, "vertex"), values(std::move(values_))

{}

void SurfaceColorVertexQuantity::createProgram() {
  // Create the program to draw this quantity
  program.reset(
      new gl::GLProgram(&VERTCOLOR3_SURFACE_VERT_SHADER, &VERTCOLOR3_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);

  setMaterialForProgram(*program, "wax");
}

void SurfaceColorVertexQuantity::fillColorBuffers(gl::GLProgram& p) {
  std::vector<Color3f> colorval;
  colorval.reserve(3 * parent.triMesh.nFaces());

  for (const HalfedgeMesh::Face& face : parent.triMesh.faces) {
    for (size_t i = 0; i < 3; i++) {
      size_t vInd = face.triangleVertices()[i]->index();
      colorval.push_back(values[vInd]);
    }
  }

  // Store data in buffers
  p.setAttribute("a_colorval", colorval);
}

void SurfaceColorVertexQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  Color3f tempColor = values[vInd];
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::string colorStr = to_string_short(tempColor);
  ImGui::TextUnformatted(colorStr.c_str());
  ImGui::NextColumn();
}

std::string SurfaceColorQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

void SurfaceColorQuantity::geometryChanged() { program.reset(); }

// ========================================================
// ==========            Face Color              ==========
// ========================================================

SurfaceColorFaceQuantity::SurfaceColorFaceQuantity(std::string name, std::vector<Color3f> values_, SurfaceMesh& mesh_)
    : SurfaceColorQuantity(name, mesh_, "face"), values(std::move(values_))

{}

void SurfaceColorFaceQuantity::createProgram() {
  // Create the program to draw this quantity
  program.reset(
      new gl::GLProgram(&VERTCOLOR3_SURFACE_VERT_SHADER, &VERTCOLOR3_SURFACE_FRAG_SHADER, gl::DrawMode::Triangles));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  fillColorBuffers(*program);

  setMaterialForProgram(*program, "wax");
}

void SurfaceColorFaceQuantity::fillColorBuffers(gl::GLProgram& p) {
  std::vector<Color3f> colorval;
  colorval.reserve(3 * parent.triMesh.nFaces());

  for (const HalfedgeMesh::Face& face : parent.triMesh.faces) {
    size_t fInd = face.index();
    for (size_t i = 0; i < 3; i++) {
      colorval.push_back(values[fInd]);
    }
  }

  // Store data in buffers
  p.setAttribute("a_colorval", colorval);
}

void SurfaceColorFaceQuantity::buildFaceInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  Color3f tempColor = values[fInd];
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << values[fInd];
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}

} // namespace polyscope
