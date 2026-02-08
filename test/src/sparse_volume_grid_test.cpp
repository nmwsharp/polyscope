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
