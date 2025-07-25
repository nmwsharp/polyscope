// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"

// ============================================================
// =============== Curve network tests
// ============================================================


TEST_F(PolyscopeTest, ShowCurveNetwork) {
  auto psCurve = registerCurveNetwork();

  // Make sure we actually added the mesh
  polyscope::show(3);
  EXPECT_TRUE(polyscope::hasCurveNetwork("test1"));
  EXPECT_FALSE(polyscope::hasCurveNetwork("test2"));
  polyscope::removeAllStructures();
  EXPECT_FALSE(polyscope::hasCurveNetwork("test1"));
}

TEST_F(PolyscopeTest, CurveNetworkAppearance) {
  auto psCurve = registerCurveNetwork();

  // Material
  psCurve->setMaterial("wax");
  EXPECT_EQ(psCurve->getMaterial(), "wax");
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkPick) {
  auto psCurve = registerCurveNetwork();

  // Don't bother trying to actually click on anything, but make sure this doesn't crash
  polyscope::pickAtBufferInds(glm::ivec2(77, 88));

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, CurveNetworkColorNode) {
  auto psCurve = registerCurveNetwork();
  std::vector<glm::vec3> vColors(psCurve->nNodes(), glm::vec3{.2, .3, .4});
  auto q1 = psCurve->addNodeColorQuantity("vcolor", vColors);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkColorFace) {
  auto psCurve = registerCurveNetwork();
  std::vector<glm::vec3> eColors(psCurve->nEdges(), glm::vec3{.2, .3, .4});
  auto q2 = psCurve->addEdgeColorQuantity("eColor", eColors);
  q2->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkScalarNode) {
  auto psCurve = registerCurveNetwork();
  std::vector<double> vScalar(psCurve->nNodes(), 7.);
  auto q1 = psCurve->addNodeScalarQuantity("vScalar", vScalar);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkScalarCategoricalNode) {
  auto psCurve = registerCurveNetwork();
  std::vector<double> vScalar(psCurve->nNodes(), 7.);
  auto q1 = psCurve->addNodeScalarQuantity("vScalar", vScalar, polyscope::DataType::CATEGORICAL);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkScalarEdge) {
  auto psCurve = registerCurveNetwork();
  std::vector<double> eScalar(psCurve->nEdges(), 9.);
  auto q3 = psCurve->addEdgeScalarQuantity("eScalar", eScalar);
  q3->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkScalarCategoricalEdge) {
  auto psCurve = registerCurveNetwork();
  std::vector<double> eScalar(psCurve->nEdges(), 9.);
  auto q3 = psCurve->addEdgeScalarQuantity("eScalar", eScalar, polyscope::DataType::CATEGORICAL);
  q3->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkNodeScalarRadius) {
  auto psCurve = registerCurveNetwork();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 10);

  std::vector<double> vScalar(psCurve->nNodes(), 0.1);
  std::vector<double> vScalar2(psCurve->nNodes(), 0.1);
  std::generate(vScalar.begin(), vScalar.end(), [&]() { return dis(gen); });
  std::generate(vScalar2.begin(), vScalar2.end(), [&]() { return dis(gen); });

  auto q1 = psCurve->addNodeScalarQuantity("vScalar", vScalar);
  auto q2 = psCurve->addNodeScalarQuantity("vScalar2", vScalar2);
  q1->setEnabled(true);

  psCurve->setNodeRadiusQuantity(q1);
  polyscope::show(3);

  psCurve->setNodeRadiusQuantity("vScalar2");
  polyscope::show(3);

  psCurve->setNodeRadiusQuantity("vScalar2", false); // no autoscaling
  polyscope::show(3);

  psCurve->clearNodeRadiusQuantity();
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkEdgeScalarRadius) {
  auto psCurve = registerCurveNetwork();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 10);

  std::vector<double> eScalar(psCurve->nEdges(), 0.1);
  std::vector<double> eScalar2(psCurve->nEdges(), 0.1);
  std::generate(eScalar.begin(), eScalar.end(), [&]() { return dis(gen); });
  std::generate(eScalar2.begin(), eScalar2.end(), [&]() { return dis(gen); });

  auto q1 = psCurve->addEdgeScalarQuantity("eScalar", eScalar);
  auto q2 = psCurve->addEdgeScalarQuantity("eScalar2", eScalar2);
  q1->setEnabled(true);

  psCurve->setEdgeRadiusQuantity(q1);
  polyscope::show(3);

  psCurve->setEdgeRadiusQuantity("eScalar2");
  polyscope::show(3);

  psCurve->setEdgeRadiusQuantity("eScalar2", false); // no autoscaling
  polyscope::show(3);

  psCurve->clearEdgeRadiusQuantity();
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkNodeAndEdgeScalarRadius) {
  auto psCurve = registerCurveNetwork();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 10);

  std::vector<double> vScalar(psCurve->nNodes(), 0.1);
  std::generate(vScalar.begin(), vScalar.end(), [&]() { return dis(gen); });
  auto q1v = psCurve->addNodeScalarQuantity("vScalar", vScalar);
  q1v->setEnabled(true);

  std::vector<double> eScalar(psCurve->nEdges(), 0.1);
  std::generate(eScalar.begin(), eScalar.end(), [&]() { return dis(gen); });
  auto q1e = psCurve->addEdgeScalarQuantity("eScalar", eScalar);
  q1e->setEnabled(true);

  psCurve->setNodeRadiusQuantity(q1v);
  psCurve->setEdgeRadiusQuantity(q1e);
  polyscope::show(3);

  psCurve->clearNodeRadiusQuantity();
  polyscope::show(3);

  psCurve->setNodeRadiusQuantity(q1v, true);
  psCurve->setEdgeRadiusQuantity(q1e, true);
  polyscope::show(3);

  psCurve->setNodeRadiusQuantity(q1v);
  psCurve->setEdgeRadiusQuantity(q1e, true);
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkVertexVector) {
  auto psCurve = registerCurveNetwork();
  std::vector<glm::vec3> vals(psCurve->nNodes(), {1., 2., 3.});
  auto q1 = psCurve->addNodeVectorQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CurveNetworkFaceVector) {
  auto psCurve = registerCurveNetwork();
  std::vector<glm::vec3> vals(psCurve->nEdges(), {1., 2., 3.});
  auto q1 = psCurve->addEdgeVectorQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}
