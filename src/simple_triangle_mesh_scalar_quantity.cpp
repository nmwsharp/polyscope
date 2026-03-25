// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/simple_triangle_mesh_scalar_quantity.h"

#include "polyscope/polyscope.h"
#include "polyscope/simple_triangle_mesh.h"

#include "imgui.h"

namespace polyscope {

SimpleTriangleMeshScalarQuantity::SimpleTriangleMeshScalarQuantity(std::string name, const std::vector<float>& values_,
                                                                   std::string definedOn_,
                                                                   SimpleTriangleMesh& mesh_, DataType dataType_)
    : SimpleTriangleMeshQuantity(name, mesh_, true), ScalarQuantity(*this, values_, dataType_),
      definedOn(definedOn_) {}

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

void SimpleTriangleMeshScalarQuantity::createProgram() {
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

void SimpleTriangleMeshScalarQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string SimpleTriangleMeshScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }

} // namespace polyscope
