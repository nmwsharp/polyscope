#include "gtest/gtest.h"

#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"

#include <array>
#include <iostream>
#include <list>
#include <string>
#include <vector>


using std::cout;
using std::endl;

class PolyscopeTest : public ::testing::Test {
protected:
  // Per-test-suite set-up.
  // Called before the first test in this test suite.
  // Can be omitted if not needed.
  static void SetUpTestSuite() { polyscope::init(); }

  // Per-test-suite tear-down.
  // Called after the last test in this test suite.
  // Can be omitted if not needed.
  /*
  static void TearDownTestSuite() {
    delete shared_resource_;
    shared_resource_ = NULL;
  }
  */

  // You can define per-test set-up logic as usual.
  // virtual void SetUp() { ... }

  // You can define per-test tear-down logic as usual.
  // virtual void TearDown() { ... }

  // Some expensive resource shared by all tests.
  // static T* shared_resource_;
};


// ============================================================
// =============== Basic tests
// ============================================================


// Show the gui. Note that the pre-suite script calls Polyscope::init() before
TEST_F(PolyscopeTest, InitializeAndShow) { polyscope::show(3); }


// ============================================================
// =============== Surface mesh tests
// ============================================================

std::tuple<std::vector<glm::vec3>, std::vector<std::vector<size_t>>> getTriangleMesh() {
  std::vector<glm::vec3> points;
  std::vector<std::vector<size_t>> faces;

  // clang-format off
  points = {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
    {0, 0, 0},
  };

  faces = {
    {1, 3, 2},
    {3, 1, 0},
    {2, 0, 1},
    {0, 2, 3}
   };
  // clang-format on

  return {points, faces};
};

polyscope::SurfaceMesh* registerTriangleMesh(std::string name = "test1") {
  std::vector<glm::vec3> points;
  std::vector<std::vector<size_t>> faces;
  std::tie(points, faces) = getTriangleMesh();
  return polyscope::registerSurfaceMesh("test1", points, faces);
}


TEST_F(PolyscopeTest, ShowSurfaceMesh) {
  auto psMesh = registerTriangleMesh();

  // Make sure we actually added the mesh
  polyscope::show(3);
  EXPECT_TRUE(polyscope::hasSurfaceMesh("test1"));
  EXPECT_FALSE(polyscope::hasSurfaceMesh("test2"));
  polyscope::removeAllStructures();
  EXPECT_FALSE(polyscope::hasSurfaceMesh("test1"));
}

TEST_F(PolyscopeTest, SurfaceMeshAppearance) {
  auto psMesh = registerTriangleMesh();

  // Both shading types
  psMesh->setSmoothShade(true);
  EXPECT_TRUE(psMesh->isSmoothShade());
  polyscope::show(3);

  psMesh->setSmoothShade(false);
  EXPECT_FALSE(psMesh->isSmoothShade());
  polyscope::show(3);

  // Wireframe
  psMesh->setEdgeWidth(1.);
  EXPECT_EQ(psMesh->getEdgeWidth(), 1.);
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshPick) {
  auto psMesh = registerTriangleMesh();

  // Don't bother trying to actually click on anything, but make sure this doesn't crash
  polyscope::pick::evaluatePickQuery(77, 88);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SurfaceMeshColorVertex) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> vColors(psMesh->nVertices(), glm::vec3{.2, .3, .4});
  auto q1 = psMesh->addVertexColorQuantity("vcolor", vColors);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshColorFace) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> fColors(psMesh->nFaces(), glm::vec3{.2, .3, .4});
  auto q2 = psMesh->addFaceColorQuantity("fColor", fColors);
  q2->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarVertex) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> vScalar(psMesh->nVertices(), 7.);
  auto q1 = psMesh->addVertexScalarQuantity("vScalar", vScalar);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarFace) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> fScalar(psMesh->nFaces(), 8.);
  auto q2 = psMesh->addFaceScalarQuantity("fScalar", fScalar);
  q2->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarEdge) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> eScalar(psMesh->nEdges(), 9.);
  auto q3 = psMesh->addEdgeScalarQuantity("eScalar", eScalar);
  q3->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarHalfedge) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> heScalar(psMesh->nHalfedges(), 10.);
  auto q4 = psMesh->addHalfedgeScalarQuantity("heScalar", heScalar);
  q4->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshDistance) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> vScalar(psMesh->nVertices(), 7.);
  auto q1 = psMesh->addVertexDistanceQuantity("distance", vScalar);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshSignedDistance) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> vScalar(psMesh->nVertices(), 7.);
  auto q1 = psMesh->addVertexSignedDistanceQuantity("distance", vScalar);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshCornerParam) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec2> vals(psMesh->nCorners(), {1., 2.});
  auto q1 = psMesh->addParameterizationQuantity("param", vals);
  q1->setEnabled(true);
  polyscope::show(3);

  // try the various render options
  q1->setStyle(ParamVizStyle::CHECKER);
  polyscope::show(3);
  q1->setStyle(ParamVizStyle::GRID);
  polyscope::show(3);
  q1->setStyle(ParamVizStyle::LOCAL_CHECK);
  polyscope::show(3);
  q1->setStyle(ParamVizStyle::LOCAL_RAD);
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshVertexParam) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec2> vals(psMesh->nVertices(), {1., 2.});
  auto q1 = psMesh->addVertexParameterizationQuantity("param", vals);
  q1->setEnabled(true);
  polyscope::show(3);

  // try the various render options
  q1->setStyle(ParamVizStyle::CHECKER);
  polyscope::show(3);
  q1->setStyle(ParamVizStyle::GRID);
  polyscope::show(3);
  q1->setStyle(ParamVizStyle::LOCAL_CHECK);
  polyscope::show(3);
  q1->setStyle(ParamVizStyle::LOCAL_RAD);
  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SurfaceMeshVertexLocalParam) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec2> vals(psMesh->nVertices(), {1., 2.});
  auto q1 = psMesh->addLocalParameterizationQuantity("param", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshVertexVector) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> vals(psMesh->nVertices(), {1., 2., 3.});
  auto q1 = psMesh->addVertexVectorQuantity("param", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshFaceVector) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> vals(psMesh->nFaces(), {1., 2., 3.});
  auto q1 = psMesh->addFaceVectorQuantity("param", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshVertexIntrinsic) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec2> vals(psMesh->nVertices(), {1., 2.});
  auto q1 = psMesh->addVertexIntrinsicVectorQuantity("param", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshFaceIntrinsic) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec2> vals(psMesh->nFaces(), {1., 2.});
  auto q1 = psMesh->addFaceIntrinsicVectorQuantity("param", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshVertexIntrinsicRibbon) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec2> vals(psMesh->nVertices(), {1., 2.});
  auto q1 = psMesh->addVertexIntrinsicVectorQuantity("param", vals);
  q1->setEnabled(true);
  q1->setRibbonEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshFaceIntrinsicRibbon) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec2> vals(psMesh->nFaces(), {1., 2.});
  auto q1 = psMesh->addFaceIntrinsicVectorQuantity("param", vals);
  q1->setEnabled(true);
  q1->setRibbonEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SurfaceMeshVertexCount) {
  auto psMesh = registerTriangleMesh();
  std::vector<std::pair<size_t, int>> vals = {{0, 1}, {2, -2}};
  auto q1 = psMesh->addVertexCountQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshFaceCount) {
  auto psMesh = registerTriangleMesh();
  std::vector<std::pair<size_t, int>> vals = {{0, 1}, {2, -2}};
  auto q1 = psMesh->addFaceCountQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshVertexIsolated) {
  auto psMesh = registerTriangleMesh();
  std::vector<std::pair<size_t, double>> vals = {{0, 1.1}, {2, -2.3}};
  auto q1 = psMesh->addVertexIsolatedScalarQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}
