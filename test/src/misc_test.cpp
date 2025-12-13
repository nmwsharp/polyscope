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


// ============================================================
// =============== Slice Plane Tests
// ============================================================

// We test these on a point cloud because it is convenient, but really we are testing the scalar quantity

TEST_F(PolyscopeTest, TestSlicePlane) {

  auto psPoints = registerPointCloud(); // add some structure to the scene

  // Basic add
  polyscope::SlicePlane* sp1 = polyscope::addSlicePlane();
  polyscope::show(3);

  // Set properties
  sp1->setColor(glm::vec3(1.0, 0.0, 0.0));
  EXPECT_EQ(sp1->getColor(), glm::vec3(1.0, 0.0, 0.0));

  sp1->setTransparency(0.5);
  EXPECT_EQ(sp1->getTransparency(), 0.5);
  
  sp1->setGridLineColor(glm::vec3(0.5, 0.5, 0.5));
  EXPECT_EQ(sp1->getGridLineColor(), glm::vec3(0.5, 0.5, 0.5));

  polyscope::show(3);

  // Transform stuff
  glm::mat4 transform = glm::mat4(1.0);
  transform[3][0] = 1.0;
  sp1->setTransform(transform);
  EXPECT_EQ(sp1->getTransform(), transform);

  glm::vec3 center = sp1->getCenter();
  glm::vec3 normal = sp1->getNormal();

  // Enable/disable drawing styles
  sp1->setDrawPlane(false);
  sp1->setDrawWidget(false);
  polyscope::show(3);

  // Add/remove with custom names
  polyscope::SlicePlane* sp2 = polyscope::addSlicePlane("custom_name");
  EXPECT_EQ(sp2->name, "custom_name");
  polyscope::SlicePlane* sp3 = polyscope::addSlicePlane();
  polyscope::show(3);
  polyscope::removeSlicePlane("custom_name");
  polyscope::show(3);
  polyscope::removeLastSceneSlicePlane();
  polyscope::show(3);
  polyscope::removeSlicePlane(sp1);
  polyscope::show(3);
  polyscope::SlicePlane* sp4 = polyscope::addSlicePlane();
  sp4->remove();
  // for now, still test that the the old deprecated function works
  polyscope::SlicePlane* sp5 = polyscope::addSceneSlicePlane();
  polyscope::show(3);

  polyscope::removeAllSlicePlanes();
  polyscope::removeAllStructures();
}