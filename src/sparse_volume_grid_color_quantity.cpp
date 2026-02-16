// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/sparse_volume_grid_color_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

// === Sparse volume grid color quantity

SparseVolumeGridColorQuantity::SparseVolumeGridColorQuantity(std::string name, SparseVolumeGrid& grid,
                                                             const std::string& definedOn_,
                                                             const std::vector<glm::vec3>& colors_)
    : SparseVolumeGridQuantity(name, grid, true), ColorQuantity(*this, colors_), definedOn(definedOn_) {}

void SparseVolumeGridColorQuantity::draw() {
  if (!isEnabled()) return;

  if (!program) createProgram();

  // Set uniforms
  parent.setSparseVolumeGridUniforms(*program);
  setColorUniforms(*program);

  render::engine->setBackfaceCull(true);
  program->draw();
}


void SparseVolumeGridColorQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string SparseVolumeGridColorQuantity::niceName() { return name + " (" + definedOn + " color)"; }


// ========================================================
// ==========            Cell Color              ==========
// ========================================================

SparseVolumeGridCellColorQuantity::SparseVolumeGridCellColorQuantity(std::string name, SparseVolumeGrid& grid,
                                                                     const std::vector<glm::vec3>& cellColors)
    : SparseVolumeGridColorQuantity(name, grid, "cell", cellColors) {}

void SparseVolumeGridCellColorQuantity::createProgram() {

  // clang-format off
  program = render::engine->requestShader("GRIDCUBE",
      render::engine->addMaterialRules(parent.getMaterial(),
        addColorRules(
          parent.addSparseGridShaderRules({
            "GRIDCUBE_PROPAGATE_ATTR_CELL_COLOR",
            "SHADE_COLOR"
          })
        )
      )
  );
  // clang-format on

  parent.setCellGeometryAttributes(*program);
  program->setAttribute("a_color", colors.getRenderAttributeBuffer());
  render::engine->setMaterial(*program, parent.getMaterial());
}


void SparseVolumeGridCellColorQuantity::buildCellInfoGUI(size_t cellInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = colors.getValue(cellInd);
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << tempColor;
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}

// ========================================================
// ==========            Node Color              ==========
// ========================================================

SparseVolumeGridNodeColorQuantity::SparseVolumeGridNodeColorQuantity(std::string name, SparseVolumeGrid& grid,
                                                                     const std::vector<glm::ivec3>& nodeIndices,
                                                                     const std::vector<glm::vec3>& nodeColors)
    : SparseVolumeGridColorQuantity(
          name, grid, "node", grid.canonicalizeNodeValueArray(name, nodeIndices, nodeColors, nodeIndicesAreCanonical)) {
}

void SparseVolumeGridNodeColorQuantity::createProgram() {

  // clang-format off
  program = render::engine->requestShader("GRIDCUBE",
      render::engine->addMaterialRules(parent.getMaterial(),
        addColorRules(
          parent.addSparseGridShaderRules({
            "GRIDCUBE_PROPAGATE_ATTR_NODE_COLOR",
            "SHADE_COLOR"
          })
        )
      )
  );
  // clang-format on

  parent.setCellGeometryAttributes(*program);
  for (int c = 0; c < 8; c++) {
    program->setAttribute("a_nodeColor" + std::to_string(c),
                          colors.getIndexedRenderAttributeBuffer(parent.cornerNodeInds[c]));
  }
  render::engine->setMaterial(*program, parent.getMaterial());
}

void SparseVolumeGridNodeColorQuantity::buildNodeInfoGUI(size_t nodeInd) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  glm::vec3 tempColor = colors.getValue(nodeInd);
  ImGui::ColorEdit3("", &tempColor[0], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoPicker);
  ImGui::SameLine();
  std::stringstream buffer;
  buffer << tempColor;
  ImGui::TextUnformatted(buffer.str().c_str());
  ImGui::NextColumn();
}
} // namespace polyscope
