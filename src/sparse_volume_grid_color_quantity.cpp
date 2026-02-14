// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/sparse_volume_grid_color_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

namespace polyscope {

// === Sparse volume grid color quantity

void SparseVolumeGridColorQuantity::draw() {
  if (!isEnabled()) return;

  if (!program) createProgram();

  // Set uniforms
  parent.setSparseVolumeGridUniforms(*program);
  setColorUniforms(*program);

  render::engine->setBackfaceCull(true);
  program->draw();
}


void SparseVolumeGridColorQuantity::createProgram() {

  // clang-format off
  program = render::engine->requestShader("GRIDCUBE",
      render::engine->addMaterialRules(parent.getMaterial(),
        addColorRules(
          parent.addSparseGridShaderRules({
            isNodeQuantity ? "GRIDCUBE_PROPAGATE_ATTR_NODE_COLOR" : "GRIDCUBE_PROPAGATE_ATTR_CELL_COLOR",
            "SHADE_COLOR"
          })
        )
      )
  );
  // clang-format on

  parent.setCellGeometryAttributes(*program);

  if (isNodeQuantity) {
    for (int c = 0; c < 8; c++) {
      program->setAttribute("a_nodeColor" + std::to_string(c),
                            colors.getIndexedRenderAttributeBuffer(parent.cornerNodeInds[c]));
    }
  } else {
    program->setAttribute("a_color", colors.getRenderAttributeBuffer());
  }

  render::engine->setMaterial(*program, parent.getMaterial());
}


void SparseVolumeGridColorQuantity::refresh() {
  program.reset();
  Quantity::refresh();
}

std::string SparseVolumeGridColorQuantity::niceName() { return name + " (color)"; }

// === Cell color quantity

SparseVolumeGridColorQuantity::SparseVolumeGridColorQuantity(std::string name, SparseVolumeGrid& grid,
                                                             const std::vector<glm::vec3>& colors_)
    : SparseVolumeGridQuantity(name, grid, true), ColorQuantity(*this, colors_), isNodeQuantity(false),
      nodeIndicesAreCanonical(true) {}


// === Node color quantity

SparseVolumeGridColorQuantity::SparseVolumeGridColorQuantity(std::string name, SparseVolumeGrid& grid,
                                                             const std::vector<glm::ivec3>& nodeIndices,
                                                             const std::vector<glm::vec3>& nodeColors)
    : SparseVolumeGridQuantity(name, grid, true),
      ColorQuantity(*this, parent.canonicalizeNodeValueArray(name, nodeIndices, nodeColors, nodeIndicesAreCanonical)),
      isNodeQuantity(true) {}

} // namespace polyscope
