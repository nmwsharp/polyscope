// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/surface_scalar_quantity.h"

#include "polyscope/file_helpers.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

namespace polyscope {

SurfaceScalarQuantity::SurfaceScalarQuantity(std::string name, SurfaceMesh& mesh_, std::string definedOn_,
                                             const std::vector<double>& values_, DataType dataType_)
    : SurfaceMeshQuantity(name, mesh_, true), ScalarQuantity(*this, values_, dataType_), definedOn(definedOn_) {}

void SurfaceScalarQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) {
    createProgram();
  }

  // Set uniforms
  parent.setStructureUniforms(*program);
  parent.setSurfaceMeshUniforms(*program);
  setScalarUniforms(*program);

  program->draw();
}


void SurfaceScalarQuantity::buildCustomUI() {
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

void SurfaceScalarQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string SurfaceScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

// ========================================================
// ==========           Vertex Scalar            ==========
// ========================================================

SurfaceVertexScalarQuantity::SurfaceVertexScalarQuantity(std::string name, const std::vector<double>& values_,
                                                         SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "vertex", values_, dataType_)

{
  values.ensureHostBufferPopulated();
  parent.vertexAreas.ensureHostBufferPopulated();
  hist.buildHistogram(values.data); // rebuild to incorporate weights
}

void SurfaceVertexScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addSurfaceMeshRules(addScalarRules({"MESH_PROPAGATE_VALUE"})));

  program->setAttribute("a_value", values.getIndexedRenderAttributeBuffer(parent.triangleVertexInds));
  parent.setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());
}


void SurfaceVertexScalarQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(vInd));
  ImGui::NextColumn();
}

// ========================================================
// ==========            Face Scalar             ==========
// ========================================================

SurfaceFaceScalarQuantity::SurfaceFaceScalarQuantity(std::string name, const std::vector<double>& values_,
                                                     SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "face", values_, dataType_)

{
  values.ensureHostBufferPopulated();
  parent.faceAreas.ensureHostBufferPopulated();
  hist.buildHistogram(values.data); // rebuild to incorporate weights
}

void SurfaceFaceScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addSurfaceMeshRules(addScalarRules({"MESH_PROPAGATE_VALUE"})));

  program->setAttribute("a_value", values.getIndexedRenderAttributeBuffer(parent.triangleFaceInds));
  parent.setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());
}


void SurfaceFaceScalarQuantity::buildFaceInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(fInd));
  ImGui::NextColumn();
}


// ========================================================
// ==========            Edge Scalar             ==========
// ========================================================

// TODO need to do something about values for internal edges in triangulated polygons

SurfaceEdgeScalarQuantity::SurfaceEdgeScalarQuantity(std::string name, const std::vector<double>& values_,
                                                     SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "edge", values_, dataType_)

{
  values.ensureHostBufferPopulated();
  hist.buildHistogram(values.data); // rebuild to incorporate weights
}

void SurfaceEdgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader(
      "MESH", parent.addSurfaceMeshRules(addScalarRules({"MESH_PROPAGATE_HALFEDGE_VALUE"})));

  program->setAttribute("a_value3", values.getIndexedRenderAttributeBuffer(parent.triangleAllEdgeInds));
  parent.setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());
}


void SurfaceEdgeScalarQuantity::buildEdgeInfoGUI(size_t eInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(eInd));
  ImGui::NextColumn();
}

// ========================================================
// ==========          Halfedge Scalar           ==========
// ========================================================

SurfaceHalfedgeScalarQuantity::SurfaceHalfedgeScalarQuantity(std::string name, const std::vector<double>& values_,
                                                             SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "halfedge", values_, dataType_)

{
  values.ensureHostBufferPopulated();
  hist.buildHistogram(values.data);
}

void SurfaceHalfedgeScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader(
      "MESH", parent.addSurfaceMeshRules(addScalarRules({"MESH_PROPAGATE_HALFEDGE_VALUE"})));

  program->setAttribute("a_value3", values.getIndexedRenderAttributeBuffer(parent.triangleAllHalfedgeInds));
  parent.setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());
}

void SurfaceHalfedgeScalarQuantity::buildHalfedgeInfoGUI(size_t heInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(heInd));
  ImGui::NextColumn();
}

// ========================================================
// ==========          Corner Scalar           ==========
// ========================================================

SurfaceCornerScalarQuantity::SurfaceCornerScalarQuantity(std::string name, const std::vector<double>& values_,
                                                         SurfaceMesh& mesh_, DataType dataType_)
    : SurfaceScalarQuantity(name, mesh_, "corner", values_, dataType_)

{
  values.ensureHostBufferPopulated();
  hist.buildHistogram(values.data);
}

void SurfaceCornerScalarQuantity::createProgram() {
  // Create the program to draw this quantity
  program = render::engine->requestShader("MESH", parent.addSurfaceMeshRules(addScalarRules({"MESH_PROPAGATE_VALUE"})));

  program->setAttribute("a_value", values.getIndexedRenderAttributeBuffer(parent.triangleCornerInds));
  parent.setMeshGeometryAttributes(*program);
  render::engine->setMaterial(*program, parent.getMaterial());
  program->setTextureFromColormap("t_colormap", cMap.get());
}

void SurfaceCornerScalarQuantity::buildCornerInfoGUI(size_t cInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(cInd));
  ImGui::NextColumn();
}

} // namespace polyscope
