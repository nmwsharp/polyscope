// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"

#include <set>
#include <tuple>

// ============================================================
// =============== Sparse volume grid tests
// ============================================================

// Helper: create a block of occupied cells from [-N, N)^3, ensuring negative indices are tested.
// Also produces matching node indices/values covering all corners of occupied cells.
struct SparseGridTestData {
  glm::vec3 origin{-3., -3., -3.};
  glm::vec3 cellWidth{0.5, 0.5, 0.5};
  std::vector<glm::ivec3> occupiedCells;
  std::vector<glm::ivec3> nodeIndices;

  // Per-cell scalar: linear index
  std::vector<float> cellScalars;
  // Per-cell color
  std::vector<glm::vec3> cellColors;
  // Per-node scalar: sum of node coords
  std::vector<float> nodeScalars;
  // Per-node color
  std::vector<glm::vec3> nodeColors;
};

SparseGridTestData buildSparseGridTestData(int N = 3) {
  SparseGridTestData d;

  // Cells from [-N, N)
  for (int i = -N; i < N; i++) {
    for (int j = -N; j < N; j++) {
      for (int k = -N; k < N; k++) {
        d.occupiedCells.push_back({i, j, k});
      }
    }
  }

  // Per-cell quantities
  d.cellScalars.resize(d.occupiedCells.size());
  d.cellColors.resize(d.occupiedCells.size());
  for (size_t i = 0; i < d.occupiedCells.size(); i++) {
    d.cellScalars[i] = static_cast<float>(i);
    glm::ivec3 ci = d.occupiedCells[i];
    d.cellColors[i] = glm::vec3((ci.x + N) / (2.f * N), (ci.y + N) / (2.f * N), (ci.z + N) / (2.f * N));
  }

  // Gather all unique node indices
  // Node (ci+dx-1, cj+dy-1, ck+dz-1) for dx,dy,dz in {0,1}
  std::set<std::tuple<int, int, int>> nodeSet;
  for (const auto& ci : d.occupiedCells) {
    for (int dx = 0; dx < 2; dx++) {
      for (int dy = 0; dy < 2; dy++) {
        for (int dz = 0; dz < 2; dz++) {
          nodeSet.insert({ci.x + dx - 1, ci.y + dy - 1, ci.z + dz - 1});
        }
      }
    }
  }

  d.nodeScalars.reserve(nodeSet.size());
  d.nodeColors.reserve(nodeSet.size());
  for (const auto& n : nodeSet) {
    int ni, nj, nk;
    std::tie(ni, nj, nk) = n;
    d.nodeIndices.push_back({ni, nj, nk});
    d.nodeScalars.push_back(static_cast<float>(ni + nj + nk));
    d.nodeColors.push_back(
        glm::vec3((ni + N) / (2.f * N + 1.f), (nj + N) / (2.f * N + 1.f), (nk + N) / (2.f * N + 1.f)));
  }

  return d;
}


TEST_F(PolyscopeTest, SparseVolumeGridShow) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  polyscope::show(3);

  EXPECT_TRUE(polyscope::hasSparseVolumeGrid("test sparse grid"));
  EXPECT_FALSE(polyscope::hasSparseVolumeGrid("other grid"));
  polyscope::removeAllStructures();
  EXPECT_FALSE(polyscope::hasSparseVolumeGrid("test sparse grid"));
}

TEST_F(PolyscopeTest, SparseVolumeGridEdges) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  psGrid->setEdgeWidth(1.f);
  psGrid->setEdgeColor({1.f, 0.f, 0.f});

  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SparseVolumeGridSlicePlane) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  polyscope::addSlicePlane();

  polyscope::show(3);

  polyscope::removeAllSlicePlanes();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SparseVolumeGridCellScalar) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  psGrid->addCellScalarQuantity("cell scalar", d.cellScalars);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridNodeScalar) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  psGrid->addNodeScalarQuantity("node scalar", d.nodeIndices, d.nodeScalars);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridCellColor) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  psGrid->addCellColorQuantity("cell color", d.cellColors);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridNodeColor) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  psGrid->addNodeColorQuantity("node color", d.nodeIndices, d.nodeColors);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridBasicOptions) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  EXPECT_EQ(psGrid->nCells(), d.occupiedCells.size());

  // Material
  psGrid->setMaterial("flat");
  EXPECT_EQ(psGrid->getMaterial(), "flat");
  polyscope::show(3);

  // Grid size factor
  psGrid->setCubeSizeFactor(0.5);
  polyscope::show(3);

  polyscope::removeAllStructures();
}
