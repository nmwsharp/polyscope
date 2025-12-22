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
// =============== Transformation Gizmo Tests
// ============================================================

TEST_F(PolyscopeTest, TransformationGizmoTest) {
  auto psMesh = registerTriangleMesh();

  // try a bunch of options for the gizmo on a structure
  psMesh->setTransformGizmoEnabled(true);
  polyscope::show(3);
  polyscope::TransformationGizmo& gizmo = psMesh->getTransformGizmo();
  gizmo.setAllowTranslation(true);
  gizmo.setAllowRotation(true);
  gizmo.setAllowScaling(true);
  gizmo.setAllowNonUniformScaling(true);
  gizmo.setInteractInLocalSpace(false);
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TransformationGizmoStandaloneTest) {

  polyscope::TransformationGizmo* gizmo1 = polyscope::addTransformationGizmo();
  polyscope::show(3);
  gizmo1->setAllowTranslation(true);
  gizmo1->setAllowRotation(true);
  gizmo1->setAllowScaling(true);
  gizmo1->setInteractInLocalSpace(false);
  gizmo1->setGizmoSize(0.5f);
  glm::mat4 T1 = gizmo1->getTransform();
  polyscope::show(3);
  polyscope::removeTransformationGizmo(gizmo1);

  // create by name
  polyscope::TransformationGizmo* gizmo2 = polyscope::addTransformationGizmo("my_gizmo");
  gizmo2->setEnabled(true);
  gizmo2->setAllowScaling(true);
  polyscope::show(3);
  polyscope::removeTransformationGizmo("my_gizmo");

  // create multiple
  polyscope::TransformationGizmo* gizmo3 = polyscope::addTransformationGizmo();
  polyscope::TransformationGizmo* gizmo4 = polyscope::addTransformationGizmo();
  polyscope::show(3);

  // non-owned transform
  glm::mat4 externalT = glm::mat4(1.0);
  externalT[0][3] = 2.0;
  polyscope::TransformationGizmo* gizmo5 = polyscope::addTransformationGizmo("my_gizmo_3", &externalT);
  EXPECT_EQ(gizmo5->getTransform(), externalT);

  glm::mat4 T = gizmo5->getTransform();
  glm::vec3 pos = gizmo5->getPosition();
  pos.z += 4.0;
  gizmo5->setPosition(pos);
  polyscope::show(3);

  polyscope::removeAllTransformationGizmos();
}

TEST_F(PolyscopeTest, TransformationGizmoNestedShowTest) {
  
  polyscope::TransformationGizmo* gizmo1 = polyscope::addTransformationGizmo();
  gizmo1->setEnabled(true);
  polyscope::show(3);

  auto showCallback = [&]() { 
    polyscope::show(3); 
  };
  polyscope::state::userCallback = showCallback;
  polyscope::show(3);

  polyscope::state::userCallback = nullptr;
  polyscope::removeAllTransformationGizmos();
}

// ============================================================
// =============== Slice Plane Tests
// ============================================================

// We test these on a point cloud because it is convenient, but really we are testing the scalar quantity

TEST_F(PolyscopeTest, TestSlicePlane) {

  auto psPoints = registerPointCloud(); // add some structure to the scene

  // Basic add
  polyscope::SlicePlane* sp1 = polyscope::addSlicePlane();
  EXPECT_TRUE(sp1->getEnabled());
  polyscope::show(3);
  sp1->setEnabled(false);
  EXPECT_FALSE(sp1->getEnabled());
  polyscope::show(3);
  sp1->setEnabled(true);

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