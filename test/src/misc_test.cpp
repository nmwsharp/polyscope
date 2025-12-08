// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"

// ============================================================
// =============== Scalar Quantity Tests
// ============================================================

// We test these on a point cloud because it is convenient, but really we are testing the scalar quantity

TEST_F(PolyscopeTest, TestScalarQuantity) {
  auto psPoints = registerPointCloud();

  std::vector<double> vScalar(psPoints->nPoints(), 7.);
  auto q1 = psPoints->addScalarQuantity("vScalar", vScalar);
  q1->setEnabled(true);
  polyscope::show(3);

  // get map range
  std::pair<double, double> newRange = {-1., 1.};
  q1->setMapRange(newRange);
  EXPECT_EQ(newRange, q1->getMapRange());

  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestScalarColormapQuantity) {
  auto psPoints = registerPointCloud();

  std::vector<double> vScalar(psPoints->nPoints(), 7.);
  auto q1 = psPoints->addScalarQuantity("vScalar", vScalar);
  q1->setEnabled(true);
  polyscope::show(3);

  // set colormap by name
  q1->setColorMap("plasma");
  EXPECT_EQ("plasma", q1->getColorMap());
  polyscope::show(3);

  // enable the onscreen colormap
  q1->setOnscreenColorbarEnabled(true);
  EXPECT_TRUE(q1->getOnscreenColorbarEnabled());
  polyscope::show(3);
  
  // set its location manually
  q1->setOnscreenColorbarLocation(glm::vec2(500.f, 500.f));
  EXPECT_EQ(glm::vec2(500.f, 500.f), q1->getOnscreenColorbarLocation());
  polyscope::show(3);

  polyscope::removeAllStructures();
}

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
