// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/simple_triangle_mesh_scalar_quantity.h"

#include "polyscope/polyscope.h"
#include "polyscope/simple_triangle_mesh.h"

#include "imgui.h"

namespace polyscope {

// ========================================================
// ==========         Scalar Quantity Base       ==========
// ========================================================

SimpleTriangleMeshScalarQuantity::SimpleTriangleMeshScalarQuantity(std::string name, const std::vector<float>& values_,
                                                                   std::string definedOn_, SimpleTriangleMesh& mesh_,
                                                                   DataType dataType_)
    : SimpleTriangleMeshQuantity(name, mesh_, true), ScalarQuantity(*this, values_, dataType_), definedOn(definedOn_) {}

void SimpleTriangleMeshScalarQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) createProgram();

  parent.setStructureUniforms(*program);
  parent.setSimpleTriangleMeshUniforms(*program);
  setScalarUniforms(*program);
  render::engine->setMaterialUniforms(*program, parent.getMaterial());

  program->draw();
}

void SimpleTriangleMeshScalarQuantity::buildCustomUI() {
  ImGui::SameLine();
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {
    buildScalarOptionsUI();
    ImGui::EndPopup();
  }
  buildScalarUI();
}

void SimpleTriangleMeshScalarQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string SimpleTriangleMeshScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }


// ========================================================
// ==========          Vertex Scalar             ==========
// ========================================================

SimpleTriangleMeshVertexScalarQuantity::SimpleTriangleMeshVertexScalarQuantity(std::string name,
                                                                               const std::vector<float>& values_,
                                                                               SimpleTriangleMesh& mesh_,
                                                                               DataType dataType_)
    : SimpleTriangleMeshScalarQuantity(name, values_, "vertex", mesh_, dataType_) {}

void SimpleTriangleMeshVertexScalarQuantity::buildVertexInfoGUI(size_t vInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(vInd));
  ImGui::NextColumn();
}

void SimpleTriangleMeshVertexScalarQuantity::createProgram() {
  // clang-format off
  program = render::engine->requestShader("SIMPLE_MESH",
    render::engine->addMaterialRules(parent.getMaterial(),
      parent.addSimpleTriangleMeshRules(
        addScalarRules(
          {"MESH_PROPAGATE_VALUE"}
        )
      )
    )
  );
  // clang-format on

  parent.setSimpleTriangleMeshProgramGeometryAttributes(*program);
  program->setAttribute("a_value", values.getRenderAttributeBuffer());
  program->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*program, parent.getMaterial());
}


// ========================================================
// ==========           Face Scalar              ==========
// ========================================================

SimpleTriangleMeshFaceScalarQuantity::SimpleTriangleMeshFaceScalarQuantity(std::string name,
                                                                           const std::vector<float>& values_,
                                                                           SimpleTriangleMesh& mesh_,
                                                                           DataType dataType_)
    : SimpleTriangleMeshScalarQuantity(name, values_, "face", mesh_, dataType_) {
  values.setTextureSize(parent.nFaces());
}

void SimpleTriangleMeshFaceScalarQuantity::buildFaceInfoGUI(size_t fInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(fInd));
  ImGui::NextColumn();
}

void SimpleTriangleMeshFaceScalarQuantity::createProgram() {
  // clang-format off
  program = render::engine->requestShader("SIMPLE_MESH",
    render::engine->addMaterialRules(parent.getMaterial(),
      parent.addSimpleTriangleMeshRules(
        addScalarRules(
          {"SIMPLE_MESH_PROPAGATE_FACE_VALUE"}
        )
      )
    )
  );
  // clang-format on

  parent.setSimpleTriangleMeshProgramGeometryAttributes(*program);
  program->setTextureFromBuffer("t_faceValues", values.getRenderTextureBuffer().get());
  program->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*program, parent.getMaterial());
}

} // namespace polyscope
