// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/sparse_volume_grid_scalar_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

// === Sparse volume grid scalar quantity

SparseVolumeGridScalarQuantity::SparseVolumeGridScalarQuantity(std::string name, SparseVolumeGrid& grid,
                                                               const std::string& definedOn_,
                                                               const std::vector<float>& values_, DataType dataType_)
    : SparseVolumeGridQuantity(name, grid, true), ScalarQuantity(*this, values_, dataType_), definedOn(definedOn_) {}

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


void SparseVolumeGridScalarQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string SparseVolumeGridScalarQuantity::niceName() { return name + " (" + definedOn + " scalar)"; }


// ========================================================
// ==========            Cell Scalar             ==========
// ========================================================

SparseVolumeGridCellScalarQuantity::SparseVolumeGridCellScalarQuantity(std::string name, SparseVolumeGrid& grid,
                                                                       const std::vector<float>& cellValues,
                                                                       DataType dataType_)
    : SparseVolumeGridScalarQuantity(name, grid, "cell", cellValues, dataType_) {}

void SparseVolumeGridCellScalarQuantity::createProgram() {

  // clang-format off
  program = render::engine->requestShader("GRIDCUBE",
      render::engine->addMaterialRules(parent.getMaterial(),
        addScalarRules(
          parent.addSparseGridShaderRules({
            "GRIDCUBE_PROPAGATE_ATTR_CELL_SCALAR"
          })
        )
      )
  );
  // clang-format on

  parent.setCellGeometryAttributes(*program);
  program->setAttribute("a_value", values.getRenderAttributeBuffer());
  program->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*program, parent.getMaterial());
}


void SparseVolumeGridCellScalarQuantity::buildCellInfoGUI(size_t cellInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(cellInd));
  ImGui::NextColumn();
}

// ========================================================
// ==========            Node Scalar             ==========
// ========================================================

SparseVolumeGridNodeScalarQuantity::SparseVolumeGridNodeScalarQuantity(std::string name, SparseVolumeGrid& grid,
                                                                       const std::vector<glm::ivec3>& nodeIndices,
                                                                       const std::vector<float>& nodeValues,
                                                                       DataType dataType_)
    : SparseVolumeGridScalarQuantity(
          name, grid, "node", grid.canonicalizeNodeValueArray(name, nodeIndices, nodeValues, nodeIndicesAreCanonical),
          dataType_) {}

void SparseVolumeGridNodeScalarQuantity::createProgram() {

  // clang-format off
  program = render::engine->requestShader("GRIDCUBE",
      render::engine->addMaterialRules(parent.getMaterial(),
        addScalarRules(
          parent.addSparseGridShaderRules({
            "GRIDCUBE_PROPAGATE_ATTR_NODE_SCALAR"
          })
        )
      )
  );
  // clang-format on

  parent.setCellGeometryAttributes(*program);
  for (int c = 0; c < 8; c++) {
    program->setAttribute("a_nodeValue" + std::to_string(c),
                          values.getIndexedRenderAttributeBuffer(parent.cornerNodeInds[c]));
  }
  program->setTextureFromColormap("t_colormap", cMap.get());
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SparseVolumeGridNodeScalarQuantity::buildNodeInfoGUI(size_t nodeInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();
  ImGui::Text("%g", values.getValue(nodeInd));
  ImGui::NextColumn();
}
} // namespace polyscope
