// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/sparse_volume_grid_scalar_quantity.h"

#include "polyscope/polyscope.h"

#include "imgui.h"

#include <unordered_map>

namespace polyscope {

// Hash for glm::ivec3
struct IVec3Hash {
  size_t operator()(const glm::ivec3& v) const {
    size_t h = std::hash<int>()(v.x);
    h ^= std::hash<int>()(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>()(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
  }
};

// === Cell scalar constructor
SparseVolumeGridScalarQuantity::SparseVolumeGridScalarQuantity(std::string name, SparseVolumeGrid& grid,
                                                               const std::vector<float>& values_, DataType dataType_)
    : SparseVolumeGridQuantity(name, grid, true), ScalarQuantity(*this, values_, dataType_), isNodeQuantity(false) {}

// === Node scalar constructor
SparseVolumeGridScalarQuantity::SparseVolumeGridScalarQuantity(std::string name, SparseVolumeGrid& grid,
                                                               const std::vector<glm::ivec3>& nodeIndices,
                                                               const std::vector<float>& nodeValues,
                                                               DataType dataType_)
    : SparseVolumeGridQuantity(name, grid, true),
      ScalarQuantity(*this, std::vector<float>(grid.nCells(), 0.f), dataType_), isNodeQuantity(true) {
  packNodeValues(nodeIndices, nodeValues);
}


void SparseVolumeGridScalarQuantity::draw() {
  if (!isEnabled()) return;

  if (!program) createProgram();

  // Set uniforms
  parent.setStructureUniforms(*program);
  program->setUniform("u_gridSpacing", parent.getGridCellWidth());
  program->setUniform("u_cubeSizeFactor", 1.f - static_cast<float>(parent.getCubeSizeFactor()));
  setScalarUniforms(*program);
  render::engine->setMaterialUniforms(*program, parent.getMaterial());

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
          parent.addStructureRules({
            isNodeQuantity ? "GRIDCUBE_PROPAGATE_ATTR_NODE_SCALAR" : "GRIDCUBE_PROPAGATE_ATTR_CELL_SCALAR"
          })
        )
      )
  );
  // clang-format on

  parent.setCellGeometryAttributes(*program);

  if (isNodeQuantity) {
    program->setAttribute("a_nodeValues04", nodeValues04->getRenderAttributeBuffer());
    program->setAttribute("a_nodeValues47", nodeValues47->getRenderAttributeBuffer());
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


void SparseVolumeGridScalarQuantity::packNodeValues(const std::vector<glm::ivec3>& nodeIndices,
                                                    const std::vector<float>& nodeValues) {
  // Build lookup map
  std::unordered_map<glm::ivec3, float, IVec3Hash> nodeMap;
  for (size_t i = 0; i < nodeIndices.size(); i++) {
    nodeMap[nodeIndices[i]] = nodeValues[i];
  }

  size_t nCells = parent.nCells();
  const std::vector<glm::ivec3>& occupiedCells = parent.getOccupiedCells();

  nodeValues04Data.resize(nCells);
  nodeValues47Data.resize(nCells);

  // Also fill values.data with per-cell averages for data range computation
  valuesData.resize(nCells);

  for (size_t i = 0; i < nCells; i++) {
    glm::ivec3 ci = occupiedCells[i];
    float vals[8];
    for (int dx = 0; dx < 2; dx++) {
      for (int dy = 0; dy < 2; dy++) {
        for (int dz = 0; dz < 2; dz++) {
          int cornerIdx = dx * 4 + dy * 2 + dz;
          glm::ivec3 nodeIjk(ci.x + dx - 1, ci.y + dy - 1, ci.z + dz - 1);
          auto it = nodeMap.find(nodeIjk);
          if (it == nodeMap.end()) {
            exception("SparseVolumeGridScalarQuantity [" + name + "]: missing node value at (" +
                      std::to_string(nodeIjk.x) + "," + std::to_string(nodeIjk.y) + "," +
                      std::to_string(nodeIjk.z) + ")");
          }
          vals[cornerIdx] = it->second;
        }
      }
    }
    nodeValues04Data[i] = glm::vec4(vals[0], vals[1], vals[2], vals[3]);
    nodeValues47Data[i] = glm::vec4(vals[4], vals[5], vals[6], vals[7]);

    // Compute average for data range
    float avg = 0;
    for (int c = 0; c < 8; c++) avg += vals[c];
    valuesData[i] = avg / 8.f;
  }

  // Update data range from the node values themselves
  if (!nodeValues.empty()) {
    float minVal = nodeValues[0], maxVal = nodeValues[0];
    for (float v : nodeValues) {
      minVal = std::min(minVal, v);
      maxVal = std::max(maxVal, v);
    }
    dataRange = {minVal, maxVal};
  }

  // Create managed buffers for the packed data
  nodeValues04 = std::unique_ptr<render::ManagedBuffer<glm::vec4>>(
      new render::ManagedBuffer<glm::vec4>(&parent, name + "#nodeValues04", nodeValues04Data));
  nodeValues47 = std::unique_ptr<render::ManagedBuffer<glm::vec4>>(
      new render::ManagedBuffer<glm::vec4>(&parent, name + "#nodeValues47", nodeValues47Data));
}


} // namespace polyscope
