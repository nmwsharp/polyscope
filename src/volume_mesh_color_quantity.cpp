// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/volume_mesh_color_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

VolumeMeshColorQuantity::VolumeMeshColorQuantity(std::string name, VolumeMesh& mesh_, std::string definedOn_,
                                                 const std::vector<glm::vec3>& colorValues_)
    : VolumeMeshQuantity(name, mesh_, true), ColorQuantity(*this, colorValues_), definedOn(definedOn_) {}


void VolumeMeshColorQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*program);
  parent.setVolumeMeshUniforms(*program);

  program->draw();
}

// ========================================================
// ==========           Vertex Color            ==========
// ========================================================

VolumeMeshVertexColorQuantity::VolumeMeshVertexColorQuantity(std::string name, VolumeMesh& mesh_,
                                                             const std::vector<glm::vec3>& values_)
    : VolumeMeshColorQuantity(name, mesh_, "vertex", values_)

{
  parent.refreshVolumeMeshListeners(); // just in case this quantity is being drawn
}

void VolumeMeshVertexColorQuantity::drawSlice(polyscope::SlicePlane* sp) {
  if (!isEnabled()) return;

  if (sliceProgram == nullptr) {
    sliceProgram = createSliceProgram();
  }
  parent.setStructureUniforms(*sliceProgram);
  // Ignore current slice plane
  sp->setSceneObjectUniforms(*sliceProgram, true);
  sp->setSliceGeomUniforms(*sliceProgram);
  parent.setVolumeMeshUniforms(*sliceProgram);
  sliceProgram->draw();
}

std::shared_ptr<render::ShaderProgram> VolumeMeshVertexColorQuantity::createSliceProgram() {

  std::shared_ptr<render::ShaderProgram> p = render::engine->requestShader(
      "SLICE_TETS", parent.addVolumeMeshRules({"SLICE_TETS_PROPAGATE_VECTOR", "SLICE_TETS_VECTOR_COLOR"}, true, true));

  // Fill color buffers
  parent.fillSliceGeometryBuffers(*p);
  fillSliceColorBuffers(*p);
  render::engine->setMaterial(*p, parent.getMaterial());
  return p;
}

void VolumeMeshVertexColorQuantity::fillSliceColorBuffers(render::ShaderProgram& p) {

  // TODO update this to use new standalone buffers

  colors.ensureHostBufferPopulated();

  size_t tetCount = parent.nTets();
  std::vector<glm::vec3> colorval_1;
  std::vector<glm::vec3> colorval_2;
  std::vector<glm::vec3> colorval_3;
  std::vector<glm::vec3> colorval_4;

  colorval_1.resize(tetCount);
  colorval_2.resize(tetCount);
  colorval_3.resize(tetCount);
  colorval_4.resize(tetCount);

  for (size_t iT = 0; iT < parent.tets.size(); iT++) {
    colorval_1[iT] = colors.data[parent.tets[iT][0]];
    colorval_2[iT] = colors.data[parent.tets[iT][1]];
    colorval_3[iT] = colors.data[parent.tets[iT][2]];
    colorval_4[iT] = colors.data[parent.tets[iT][3]];
  }

  // Store data in buffers
  p.setAttribute("a_value_1", colorval_1);
  p.setAttribute("a_value_2", colorval_2);
  p.setAttribute("a_value_3", colorval_3);
  p.setAttribute("a_value_4", colorval_4);
}

void VolumeMeshVertexColorQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addVolumeMeshRules({"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  program->setAttribute("a_color", colors.getIndexedRenderAttributeBuffer(parent.triangleVertexInds));
  render::engine->setMaterial(*program, parent.getMaterial());
}

void VolumeMeshVertexColorQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = colors.getValue(vInd);
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::string colorStr = to_string_short(tempColor);
  ImGui::TextUnformatted(colorStr.c_str());
  ImGui::NextColumn();
}

std::string VolumeMeshColorQuantity::niceName() { return name + " (" + definedOn + " color)"; }

void VolumeMeshColorQuantity::refresh() {
  program.reset();
  if (sliceProgram) {
    sliceProgram.reset();
  }
  Quantity::refresh();
}

// ========================================================
// ==========            Cell Color              ==========
// ========================================================

VolumeMeshCellColorQuantity::VolumeMeshCellColorQuantity(std::string name, VolumeMesh& mesh_,
                                                         const std::vector<glm::vec3>& values_)
    : VolumeMeshColorQuantity(name, mesh_, "cell", values_)

{}

void VolumeMeshCellColorQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addVolumeMeshRules({"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}));

  // Fill color buffers
  parent.fillGeometryBuffers(*program);
  program->setAttribute("a_color", colors.getIndexedRenderAttributeBuffer(parent.triangleCellInds));
  render::engine->setMaterial(*program, parent.getMaterial());
}

void VolumeMeshCellColorQuantity::buildCellInfoGUI(size_t fInd) {
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
