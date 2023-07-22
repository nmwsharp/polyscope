// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/types.h"
#include "polyscope_test.h"

#include "polyscope/curve_network.h"
#include "polyscope/pick.h"
#include "polyscope/point_cloud.h"
#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/volume_mesh.h"

#include "gtest/gtest.h"

#include <array>
#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "polyscope_test.h"

// ============================================================
// =============== Point cloud tests
// ============================================================

TEST_F(PolyscopeTest, ShowPointCloud) {
  auto psPoints = registerPointCloud();

  polyscope::show(3);
  EXPECT_TRUE(polyscope::hasPointCloud("test1"));
  EXPECT_FALSE(polyscope::hasPointCloud("test2"));
  polyscope::removeAllStructures();
  EXPECT_FALSE(polyscope::hasPointCloud("test1"));
}

TEST_F(PolyscopeTest, PointCloudUpdateGeometry) {
  auto psPoints = registerPointCloud();
  polyscope::show(3);

  psPoints->updatePointPositions(getPoints());
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, PointCloudAppearance) {
  auto psPoints = registerPointCloud();

  // Radius
  psPoints->setPointRadius(0.02);
  polyscope::show(3);

  // Material
  psPoints->setMaterial("wax");
  EXPECT_EQ(psPoints->getMaterial(), "wax");
  polyscope::show(3);

  psPoints->setPointRenderMode(polyscope::PointRenderMode::Quad);
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, PointCloudPick) {
  auto psPoints = registerPointCloud();

  // Don't bother trying to actually click on anything, but make sure this doesn't crash
  polyscope::pick::evaluatePickQuery(77, 88);

  psPoints->setPointRenderMode(polyscope::PointRenderMode::Quad);
  polyscope::pick::evaluatePickQuery(77, 88);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, PointCloudColor) {
  auto psPoints = registerPointCloud();
  std::vector<glm::vec3> vColors(psPoints->nPoints(), glm::vec3{.2, .3, .4});
  auto q1 = psPoints->addColorQuantity("vcolor", vColors);
  q1->setEnabled(true);
  polyscope::show(3);

  psPoints->setPointRenderMode(polyscope::PointRenderMode::Quad);
  polyscope::show(3);

  q1->updateData(vColors);
  polyscope::show(3);

  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, PointCloudScalar) {
  auto psPoints = registerPointCloud();

  std::vector<double> vScalar(psPoints->nPoints(), 7.);
  auto q1 = psPoints->addScalarQuantity("vScalar", vScalar);
  q1->setEnabled(true);
  polyscope::show(3);

  psPoints->setPointRenderMode(polyscope::PointRenderMode::Quad);
  polyscope::show(3);

  q1->updateData(vScalar);
  polyscope::show(3);

  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, PointCloudVector) {
  auto psPoints = registerPointCloud();

  std::vector<glm::vec3> vals(psPoints->nPoints(), {1., 2., 3.});
  auto q1 = psPoints->addVectorQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);

  q1->updateData(vals);
  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, PointCloudParam) {
  auto psPoints = registerPointCloud();
  std::vector<glm::vec2> param(psPoints->nPoints(), glm::vec2{.2, .3});

  auto q1 = psPoints->addParameterizationQuantity("param", param);
  q1->setEnabled(true);
  polyscope::show(3);

  psPoints->setPointRenderMode(polyscope::PointRenderMode::Quad);
  polyscope::show(3);

  auto q2 = psPoints->addLocalParameterizationQuantity("local param", param);
  psPoints->setPointRenderMode(polyscope::PointRenderMode::Sphere);
  q2->setEnabled(true);
  polyscope::show(3);

  psPoints->setPointRenderMode(polyscope::PointRenderMode::Quad);
  polyscope::show(3);

  q1->updateCoords(param);
  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, PointCloudScalarRadius) {
  auto psPoints = registerPointCloud();
  std::vector<double> vScalar(psPoints->nPoints(), 7.);
  std::vector<double> vScalar2(psPoints->nPoints(), 7.);
  auto q1 = psPoints->addScalarQuantity("vScalar", vScalar);
  auto q2 = psPoints->addScalarQuantity("vScalar2", vScalar2);
  q1->setEnabled(true);

  psPoints->setPointRadiusQuantity(q1);
  polyscope::show(3);

  psPoints->setPointRenderMode(polyscope::PointRenderMode::Quad);
  polyscope::show(3);

  psPoints->setPointRadiusQuantity("vScalar2");
  polyscope::show(3);

  psPoints->setPointRadiusQuantity("vScalar2", false); // no autoscaling
  polyscope::show(3);

  q2->updateData(vScalar2);
  polyscope::show(3);

  psPoints->clearPointRadiusQuantity();
  polyscope::show(3);

  polyscope::removeAllStructures();
}
