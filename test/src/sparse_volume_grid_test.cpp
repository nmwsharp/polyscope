// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"


// ============================================================
// =============== Sparse volume grid tests
// ============================================================

TEST_F(PolyscopeTest, SparseVolumeGridShow) {
  glm::vec3 origin{-3., -3., -3.};
  glm::vec3 cellWidth{0.5, 0.5, 0.5};

  // Create some occupied cells
  std::vector<glm::ivec3> occupiedCells;
  for (uint32_t i = 0; i < 8; i += 2) {
    for (uint32_t j = 0; j < 10; j += 2) {
      for (uint32_t k = 0; k < 12; k += 2) {
        occupiedCells.push_back({i, j, k});
      }
    }
  }

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", origin, cellWidth, occupiedCells);

  polyscope::show(3);

  EXPECT_TRUE(polyscope::hasSparseVolumeGrid("test sparse grid"));
  EXPECT_FALSE(polyscope::hasSparseVolumeGrid("other grid"));
  polyscope::removeAllStructures();
  EXPECT_FALSE(polyscope::hasSparseVolumeGrid("test sparse grid"));
}


TEST_F(PolyscopeTest, SparseVolumeGridCellScalar) {
  glm::vec3 origin{-3., -3., -3.};
  glm::vec3 cellWidth{0.5, 0.5, 0.5};

  std::vector<glm::ivec3> occupiedCells;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        occupiedCells.push_back({i, j, k});
      }
    }
  }

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", origin, cellWidth, occupiedCells);

  std::vector<float> scalarVals(occupiedCells.size());
  for (size_t i = 0; i < scalarVals.size(); i++) {
    scalarVals[i] = static_cast<float>(i);
  }
  psGrid->addCellScalarQuantity("cell scalar", scalarVals);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridNodeScalar) {
  glm::vec3 origin{-3., -3., -3.};
  glm::vec3 cellWidth{0.5, 0.5, 0.5};

  std::vector<glm::ivec3> occupiedCells;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        occupiedCells.push_back({i, j, k});
      }
    }
  }

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", origin, cellWidth, occupiedCells);

  // Node indices: corners of each cell are at (ci+dx-1, cj+dy-1, ck+dz-1) for dx,dy,dz in {0,1}
  // For cells (0..3)^3, nodes range from -1..3
  std::vector<glm::ivec3> nodeIndices;
  std::vector<float> nodeValues;
  for (int i = -1; i <= 3; i++) {
    for (int j = -1; j <= 3; j++) {
      for (int k = -1; k <= 3; k++) {
        nodeIndices.push_back({i, j, k});
        nodeValues.push_back(static_cast<float>(i + j + k));
      }
    }
  }
  psGrid->addNodeScalarQuantity("node scalar", nodeIndices, nodeValues);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridCellColor) {
  glm::vec3 origin{-3., -3., -3.};
  glm::vec3 cellWidth{0.5, 0.5, 0.5};

  std::vector<glm::ivec3> occupiedCells;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        occupiedCells.push_back({i, j, k});
      }
    }
  }

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", origin, cellWidth, occupiedCells);

  std::vector<glm::vec3> colorVals(occupiedCells.size());
  for (size_t i = 0; i < colorVals.size(); i++) {
    colorVals[i] = glm::vec3(static_cast<float>(i) / colorVals.size(), 0.5f, 0.3f);
  }
  psGrid->addCellColorQuantity("cell color", colorVals);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridNodeColor) {
  glm::vec3 origin{-3., -3., -3.};
  glm::vec3 cellWidth{0.5, 0.5, 0.5};

  std::vector<glm::ivec3> occupiedCells;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        occupiedCells.push_back({i, j, k});
      }
    }
  }

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", origin, cellWidth, occupiedCells);

  // Node indices: corners of each cell are at (ci+dx-1, cj+dy-1, ck+dz-1) for dx,dy,dz in {0,1}
  std::vector<glm::ivec3> nodeIndices;
  std::vector<glm::vec3> nodeColors;
  for (int i = -1; i <= 3; i++) {
    for (int j = -1; j <= 3; j++) {
      for (int k = -1; k <= 3; k++) {
        nodeIndices.push_back({i, j, k});
        nodeColors.push_back(glm::vec3((i + 1) / 4.f, (j + 1) / 4.f, (k + 1) / 4.f));
      }
    }
  }
  psGrid->addNodeColorQuantity("node color", nodeIndices, nodeColors);

  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SparseVolumeGridBasicOptions) {
  glm::vec3 origin{-3., -3., -3.};
  glm::vec3 cellWidth{0.5, 0.5, 0.5};

  std::vector<glm::ivec3> occupiedCells;
  for (uint32_t i = 0; i < 8; i += 2) {
    for (uint32_t j = 0; j < 10; j += 2) {
      for (uint32_t k = 0; k < 12; k += 2) {
        occupiedCells.push_back({i, j, k});
      }
    }
  }

  polyscope::SparseVolumeGrid* psGrid =
      polyscope::registerSparseVolumeGrid("test sparse grid", origin, cellWidth, occupiedCells);

  EXPECT_EQ(psGrid->nCells(), occupiedCells.size());

  // Material
  psGrid->setMaterial("flat");
  EXPECT_EQ(psGrid->getMaterial(), "flat");
  polyscope::show(3);

  // Grid size factor
  psGrid->setCubeSizeFactor(0.5);
  polyscope::show(3);

  polyscope::removeAllStructures();
}
