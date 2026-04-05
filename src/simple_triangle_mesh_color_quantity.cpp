// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/simple_triangle_mesh_color_quantity.h"

#include "polyscope/polyscope.h"
#include "polyscope/simple_triangle_mesh.h"

#include "imgui.h"

namespace polyscope {

// ========================================================
// ==========         Color Quantity Base        ==========
// ========================================================

SimpleTriangleMeshColorQuantity::SimpleTriangleMeshColorQuantity(std::string name,
                                                                 const std::vector<glm::vec3>& colors_,
                                                                 std::string definedOn_,
                                                                 SimpleTriangleMesh& mesh_)
    : SimpleTriangleMeshQuantity(name, mesh_, true), ColorQuantity(*this, colors_), definedOn(definedOn_) {}

void SimpleTriangleMeshColorQuantity::draw() {
  if (!isEnabled()) return;

  if (program == nullptr) createProgram();

  parent.setStructureUniforms(*program);
  parent.setSimpleTriangleMeshUniforms(*program);
  setColorUniforms(*program);
  render::engine->setMaterialUniforms(*program, parent.getMaterial());

  program->draw();
}

void SimpleTriangleMeshColorQuantity::buildCustomUI() {
  ImGui::SameLine();
  if (ImGui::Button("Options")) {
    ImGui::OpenPopup("OptionsPopup");
  }
  if (ImGui::BeginPopup("OptionsPopup")) {
    buildColorOptionsUI();
    ImGui::EndPopup();
  }
  buildColorUI();
}

void SimpleTriangleMeshColorQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string SimpleTriangleMeshColorQuantity::niceName() { return name + " (" + definedOn + " color)"; }


// ========================================================
// ==========          Vertex Color              ==========
// ========================================================

SimpleTriangleMeshVertexColorQuantity::SimpleTriangleMeshVertexColorQuantity(std::string name,
                                                                             const std::vector<glm::vec3>& colors_,
                                                                             SimpleTriangleMesh& mesh_)
    : SimpleTriangleMeshColorQuantity(name, colors_, "vertex", mesh_) {}

void SimpleTriangleMeshVertexColorQuantity::createProgram() {
  // clang-format off
  program = render::engine->requestShader("SIMPLE_MESH",
    render::engine->addMaterialRules(parent.getMaterial(),
      addColorRules(
        parent.addSimpleTriangleMeshRules(
          {"MESH_PROPAGATE_COLOR", "SHADE_COLOR"}
        )
      )
    )
  );
  // clang-format on

  parent.setSimpleTriangleMeshProgramGeometryAttributes(*program);
  program->setAttribute("a_color", colors.getRenderAttributeBuffer());
  render::engine->setMaterial(*program, parent.getMaterial());
}


// ========================================================
// ==========           Face Color               ==========
// ========================================================

SimpleTriangleMeshFaceColorQuantity::SimpleTriangleMeshFaceColorQuantity(std::string name,
                                                                         const std::vector<glm::vec3>& colors_,
                                                                         SimpleTriangleMesh& mesh_)
    : SimpleTriangleMeshColorQuantity(name, colors_, "face", mesh_) {
  colors.setTextureSize(parent.nFaces());
}

void SimpleTriangleMeshFaceColorQuantity::createProgram() {
  // clang-format off
  program = render::engine->requestShader("SIMPLE_MESH",
    render::engine->addMaterialRules(parent.getMaterial(),
      addColorRules(
        parent.addSimpleTriangleMeshRules(
          {"SIMPLE_MESH_PROPAGATE_FACE_COLOR", "SHADE_COLOR"}
        )
      )
    )
  );
  // clang-format on

  parent.setSimpleTriangleMeshProgramGeometryAttributes(*program);
  program->setTextureFromBuffer("t_faceColors", colors.getRenderTextureBuffer().get());
  render::engine->setMaterial(*program, parent.getMaterial());
}

} // namespace polyscope
