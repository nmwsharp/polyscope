// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/sparse_volume_grid_color_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

#include <unordered_map>

namespace polyscope {

// Hash for glm::ivec3
struct IVec3HashColor {
  size_t operator()(const glm::ivec3& v) const {
    size_t h = std::hash<int>()(v.x);
    h ^= std::hash<int>()(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>()(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
  }
};

// === Cell color constructor
SparseVolumeGridColorQuantity::SparseVolumeGridColorQuantity(std::string name, SparseVolumeGrid& grid,
                                                             const std::vector<glm::vec3>& colors_)
    : SparseVolumeGridQuantity(name, grid, true), ColorQuantity(*this, colors_), isNodeQuantity(false) {}

// === Node color constructor
SparseVolumeGridColorQuantity::SparseVolumeGridColorQuantity(std::string name, SparseVolumeGrid& grid,
                                                             const std::vector<glm::ivec3>& nodeIndices,
                                                             const std::vector<glm::vec3>& nodeColors)
    : SparseVolumeGridQuantity(name, grid, true),
      ColorQuantity(*this, std::vector<glm::vec3>(grid.nCells(), glm::vec3{0.f, 0.f, 0.f})), isNodeQuantity(true) {
  packNodeColors(nodeIndices, nodeColors);
}


void SparseVolumeGridColorQuantity::draw() {
  if (!isEnabled()) return;

  if (!program) createProgram();

  // Set uniforms
  parent.setStructureUniforms(*program);
  program->setUniform("u_gridSpacing", parent.getGridCellWidth());
  program->setUniform("u_cubeSizeFactor", 1.f - static_cast<float>(parent.getCubeSizeFactor()));
  setColorUniforms(*program);
  render::engine->setMaterialUniforms(*program, parent.getMaterial());

  render::engine->setBackfaceCull(true);
  program->draw();
}


void SparseVolumeGridColorQuantity::createProgram() {

  // clang-format off
  program = render::engine->requestShader("GRIDCUBE",
      render::engine->addMaterialRules(parent.getMaterial(),
        addColorRules(
          parent.addStructureRules({
            isNodeQuantity ? "GRIDCUBE_PROPAGATE_ATTR_NODE_COLOR" : "GRIDCUBE_PROPAGATE_ATTR_CELL_COLOR",
            "SHADE_COLOR"
          })
        )
      )
  );
  // clang-format on

  parent.setCellGeometryAttributes(*program);

  if (isNodeQuantity) {
    program->setAttribute("a_nodeR04", nodeR04->getRenderAttributeBuffer());
    program->setAttribute("a_nodeR47", nodeR47->getRenderAttributeBuffer());
    program->setAttribute("a_nodeG04", nodeG04->getRenderAttributeBuffer());
    program->setAttribute("a_nodeG47", nodeG47->getRenderAttributeBuffer());
    program->setAttribute("a_nodeB04", nodeB04->getRenderAttributeBuffer());
    program->setAttribute("a_nodeB47", nodeB47->getRenderAttributeBuffer());
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


void SparseVolumeGridColorQuantity::packNodeColors(const std::vector<glm::ivec3>& nodeIndices,
                                                   const std::vector<glm::vec3>& nodeColors) {
  // Build lookup map
  std::unordered_map<glm::ivec3, glm::vec3, IVec3HashColor> nodeMap;
  for (size_t i = 0; i < nodeIndices.size(); i++) {
    nodeMap[nodeIndices[i]] = nodeColors[i];
  }

  size_t nCells = parent.nCells();
  const std::vector<glm::ivec3>& occupiedCells = parent.getOccupiedCells();

  nodeR04Data.resize(nCells);
  nodeR47Data.resize(nCells);
  nodeG04Data.resize(nCells);
  nodeG47Data.resize(nCells);
  nodeB04Data.resize(nCells);
  nodeB47Data.resize(nCells);

  for (size_t i = 0; i < nCells; i++) {
    glm::ivec3 ci = occupiedCells[i];
    glm::vec3 cornerColors[8];
    for (int dx = 0; dx < 2; dx++) {
      for (int dy = 0; dy < 2; dy++) {
        for (int dz = 0; dz < 2; dz++) {
          int cornerIdx = dx * 4 + dy * 2 + dz;
          glm::ivec3 nodeIjk(ci.x + dx - 1, ci.y + dy - 1, ci.z + dz - 1);
          auto it = nodeMap.find(nodeIjk);
          if (it == nodeMap.end()) {
            exception("SparseVolumeGridColorQuantity [" + name + "]: missing node color at (" +
                      std::to_string(nodeIjk.x) + "," + std::to_string(nodeIjk.y) + "," +
                      std::to_string(nodeIjk.z) + ")");
          }
          cornerColors[cornerIdx] = it->second;
        }
      }
    }

    // Pack by channel: R, G, B each get 2 vec4 (corners 0-3, 4-7)
    nodeR04Data[i] = glm::vec4(cornerColors[0].r, cornerColors[1].r, cornerColors[2].r, cornerColors[3].r);
    nodeR47Data[i] = glm::vec4(cornerColors[4].r, cornerColors[5].r, cornerColors[6].r, cornerColors[7].r);
    nodeG04Data[i] = glm::vec4(cornerColors[0].g, cornerColors[1].g, cornerColors[2].g, cornerColors[3].g);
    nodeG47Data[i] = glm::vec4(cornerColors[4].g, cornerColors[5].g, cornerColors[6].g, cornerColors[7].g);
    nodeB04Data[i] = glm::vec4(cornerColors[0].b, cornerColors[1].b, cornerColors[2].b, cornerColors[3].b);
    nodeB47Data[i] = glm::vec4(cornerColors[4].b, cornerColors[5].b, cornerColors[6].b, cornerColors[7].b);
  }

  // Create managed buffers for the packed data
  nodeR04 = std::unique_ptr<render::ManagedBuffer<glm::vec4>>(
      new render::ManagedBuffer<glm::vec4>(&parent, name + "#nodeR04", nodeR04Data));
  nodeR47 = std::unique_ptr<render::ManagedBuffer<glm::vec4>>(
      new render::ManagedBuffer<glm::vec4>(&parent, name + "#nodeR47", nodeR47Data));
  nodeG04 = std::unique_ptr<render::ManagedBuffer<glm::vec4>>(
      new render::ManagedBuffer<glm::vec4>(&parent, name + "#nodeG04", nodeG04Data));
  nodeG47 = std::unique_ptr<render::ManagedBuffer<glm::vec4>>(
      new render::ManagedBuffer<glm::vec4>(&parent, name + "#nodeG47", nodeG47Data));
  nodeB04 = std::unique_ptr<render::ManagedBuffer<glm::vec4>>(
      new render::ManagedBuffer<glm::vec4>(&parent, name + "#nodeB04", nodeB04Data));
  nodeB47 = std::unique_ptr<render::ManagedBuffer<glm::vec4>>(
      new render::ManagedBuffer<glm::vec4>(&parent, name + "#nodeB47", nodeB47Data));
}


} // namespace polyscope
