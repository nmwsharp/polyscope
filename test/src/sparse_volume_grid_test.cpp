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
  // Node (ci+dx, cj+dy, ck+dz) for dx,dy,dz in {0,1}
  std::set<std::tuple<int, int, int>> nodeSet;
  for (const auto& ci : d.occupiedCells) {
    for (int dx = 0; dx < 2; dx++) {
      for (int dy = 0; dy < 2; dy++) {
        for (int dz = 0; dz < 2; dz++) {
          nodeSet.insert({ci.x + dx, ci.y + dy, ci.z + dz});
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

TEST_F(PolyscopeTest, SparseVolumeGridPick) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  // Don't bother trying to actually click on anything, but make sure this doesn't crash
  polyscope::pickAtBufferInds(glm::ivec2(77, 88));

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridNodeIndexingConvention) {
  // Verify that cell (i,j,k) has corner nodes at (i+dx, j+dy, k+dz) for dx,dy,dz in {0,1}.
  // We register a single cell at (0,0,0), then provide node values at exactly those 8 corners
  // in canonical (sorted) order. If the indexing convention is correct, getNodeIndicesAreCanonical()
  // should return true.

  std::vector<glm::ivec3> cells = {{0, 0, 0}};
  glm::vec3 origin{0.f, 0.f, 0.f};
  glm::vec3 cellWidth{1.f, 1.f, 1.f};

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("index test grid", origin, cellWidth, cells);

  // The 8 corner nodes of cell (0,0,0) should be (0,0,0) through (1,1,1), in lexicographic order
  std::vector<glm::ivec3> nodeIndices = {
      {0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {1, 0, 1}, {1, 1, 0}, {1, 1, 1},
  };
  std::vector<float> nodeScalars(8, 1.f);

  auto* q = psGrid->addNodeScalarQuantity("index check", nodeIndices, nodeScalars);
  EXPECT_TRUE(q->getNodeIndicesAreCanonical());

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SparseVolumeGridCellScalar) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  polyscope::SparseVolumeGridScalarQuantity* q = psGrid->addCellScalarQuantity("cell scalar", d.cellScalars);
  q->setEnabled(true);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridNodeScalar) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  polyscope::SparseVolumeGridScalarQuantity* q =
      psGrid->addNodeScalarQuantity("node scalar", d.nodeIndices, d.nodeScalars);
  q->setEnabled(true);


  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridCellColor) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  polyscope::SparseVolumeGridCellColorQuantity* q = psGrid->addCellColorQuantity("cell color", d.cellColors);
  q->setEnabled(true);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridNodeColor) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  polyscope::SparseVolumeGridNodeColorQuantity* q =
      psGrid->addNodeColorQuantity("node color", d.nodeIndices, d.nodeColors);
  q->setEnabled(true);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridDuplicateCellsThrows) {
  auto d = buildSparseGridTestData();

  // Add a duplicate cell
  std::vector<glm::ivec3> cellsWithDup = d.occupiedCells;
  cellsWithDup.push_back(d.occupiedCells[0]);

  EXPECT_THROW(polyscope::registerSparseVolumeGrid("dup grid", d.origin, d.cellWidth, cellsWithDup), std::logic_error);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridNodeMissingValuesThrows) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  // Drop the last node to create a missing entry
  std::vector<glm::ivec3> partialIndices(d.nodeIndices.begin(), d.nodeIndices.end() - 1);
  std::vector<float> partialScalars(d.nodeScalars.begin(), d.nodeScalars.end() - 1);
  std::vector<glm::vec3> partialColors(d.nodeColors.begin(), d.nodeColors.end() - 1);

  EXPECT_THROW(psGrid->addNodeScalarQuantity("missing scalar", partialIndices, partialScalars), std::runtime_error);
  EXPECT_THROW(psGrid->addNodeColorQuantity("missing color", partialIndices, partialColors), std::runtime_error);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridNodeExtraValuesOk) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  // Add extra node entries not present in the grid
  std::vector<glm::ivec3> extraIndices = d.nodeIndices;
  std::vector<float> extraScalars = d.nodeScalars;
  std::vector<glm::vec3> extraColors = d.nodeColors;
  extraIndices.push_back({999, 999, 999});
  extraScalars.push_back(0.f);
  extraColors.push_back({0.f, 0.f, 0.f});

  // Should not throw
  EXPECT_NO_THROW(psGrid->addNodeScalarQuantity("extra scalar", extraIndices, extraScalars));
  EXPECT_NO_THROW(psGrid->addNodeColorQuantity("extra color", extraIndices, extraColors));

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridNodeCanonicalFlag) {
  auto d = buildSparseGridTestData();

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", d.origin, d.cellWidth, d.occupiedCells);

  // The test data's nodeIndices come from std::set iteration, which is sorted — should match canonical order
  auto* qScalar = psGrid->addNodeScalarQuantity("canonical scalar", d.nodeIndices, d.nodeScalars);
  EXPECT_TRUE(qScalar->getNodeIndicesAreCanonical());

  auto* qColor = psGrid->addNodeColorQuantity("canonical color", d.nodeIndices, d.nodeColors);
  EXPECT_TRUE(qColor->getNodeIndicesAreCanonical());

  // Now provide the same data in reversed order — should NOT be canonical
  std::vector<glm::ivec3> reversedIndices(d.nodeIndices.rbegin(), d.nodeIndices.rend());
  std::vector<float> reversedScalars(d.nodeScalars.rbegin(), d.nodeScalars.rend());
  std::vector<glm::vec3> reversedColors(d.nodeColors.rbegin(), d.nodeColors.rend());

  auto* qScalar2 = psGrid->addNodeScalarQuantity("reversed scalar", reversedIndices, reversedScalars);
  EXPECT_FALSE(qScalar2->getNodeIndicesAreCanonical());

  auto* qColor2 = psGrid->addNodeColorQuantity("reversed color", reversedIndices, reversedColors);
  EXPECT_FALSE(qColor2->getNodeIndicesAreCanonical());

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

  // Voxel render mode
  EXPECT_EQ(psGrid->getRenderMode(), polyscope::SparseVolumeGridRenderMode::Gridcube);
  psGrid->setRenderMode(polyscope::SparseVolumeGridRenderMode::Wireframe);
  EXPECT_EQ(psGrid->getRenderMode(), polyscope::SparseVolumeGridRenderMode::Wireframe);
  polyscope::show(3);

  // Wireframe radius
  psGrid->setWireframeRadius(2.0);
  EXPECT_DOUBLE_EQ(psGrid->getWireframeRadius(), 2.0);
  polyscope::show(3);

  psGrid->setWireframeRadius(0.5);
  EXPECT_DOUBLE_EQ(psGrid->getWireframeRadius(), 0.5);
  polyscope::show(3);

  // Wireframe color
  psGrid->setWireframeColor({1.f, 0.f, 0.f});
  glm::vec3 wfColor = psGrid->getWireframeColor();
  EXPECT_FLOAT_EQ(wfColor.x, 1.f);
  EXPECT_FLOAT_EQ(wfColor.y, 0.f);
  EXPECT_FLOAT_EQ(wfColor.z, 0.f);
  polyscope::show(3);

  psGrid->setRenderMode(polyscope::SparseVolumeGridRenderMode::Gridcube);
  EXPECT_EQ(psGrid->getRenderMode(), polyscope::SparseVolumeGridRenderMode::Gridcube);
  polyscope::show(3);

  polyscope::removeAllStructures();
}
