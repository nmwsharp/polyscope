// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/sparse_volume_grid_scalar_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

// === SparseVolumeGridScalarQuantity

void SparseVolumeGridScalarQuantity::draw() {
  if (!isEnabled()) return;

  if (!program) createProgram();

  // Set uniforms
  parent.setSparseVolumeGridUniforms(*program);
  setScalarUniforms(*program);

  render::engine->setBackfaceCull(true);
  program->draw();
}


void SparseVolumeGridScalarQuantity::buildCustomUI() {
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


void SparseVolumeGridScalarQuantity::createProgram() {

  // clang-format off
  program = render::engine->requestShader("GRIDCUBE",
      render::engine->addMaterialRules(parent.getMaterial(),
        addScalarRules(
          parent.addSparseGridShaderRules({
            isNodeQuantity ? "GRIDCUBE_PROPAGATE_ATTR_NODE_SCALAR" : "GRIDCUBE_PROPAGATE_ATTR_CELL_SCALAR"
          })
        )
      )
  );
  // clang-format on

  parent.setCellGeometryAttributes(*program);

  if (isNodeQuantity) {
    for (int c = 0; c < 8; c++) {
      program->setAttribute("a_nodeValue" + std::to_string(c),
                            values.getIndexedRenderAttributeBuffer(parent.cornerNodeInds[c]));
    }
  } else {
    program->setAttribute("a_value", values.getRenderAttributeBuffer());
  }

  program->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*program, parent.getMaterial());
}


void SparseVolumeGridScalarQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string SparseVolumeGridScalarQuantity::niceName() { return name + " (scalar)"; }


// === Cell scalar quantity

SparseVolumeGridScalarQuantity::SparseVolumeGridScalarQuantity(std::string name, SparseVolumeGrid& grid,
                                                               const std::vector<float>& values_, DataType dataType_)
    : SparseVolumeGridQuantity(name, grid, true), ScalarQuantity(*this, values_, dataType_), isNodeQuantity(false),
      nodeIndicesAreCanonical(true) {}


// === Node scalar quantity

SparseVolumeGridScalarQuantity::SparseVolumeGridScalarQuantity(std::string name, SparseVolumeGrid& grid,
                                                               const std::vector<glm::ivec3>& nodeIndices,
                                                               const std::vector<float>& nodeValues, DataType dataType_)
    : SparseVolumeGridQuantity(name, grid, true),
      ScalarQuantity(*this, parent.canonicalizeNodeValueArray(name, nodeIndices, nodeValues, nodeIndicesAreCanonical),
                     dataType_),
      isNodeQuantity(true) {}


} // namespace polyscope
