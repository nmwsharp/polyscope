// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"

// ============================================================
// =============== Materials tests
// ============================================================

TEST_F(PolyscopeTest, FlatMaterialTest) {
  auto psMesh = registerTriangleMesh();

  // Test the flat material, it uses custom rules & uniforms and thus is different from others
  psMesh->setMaterial("flat");
  EXPECT_EQ(psMesh->getMaterial(), "flat");
  polyscope::show(3);

  polyscope::removeAllStructures();
}
