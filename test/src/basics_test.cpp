
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


using std::cout;
using std::endl;

class PolyscopeTest : public ::testing::Test {
protected:
  // Per-test-suite set-up.
  // Called before the first test in this test suite.
  // Can be omitted if not needed.
  static void SetUpTestSuite() { polyscope::init(testBackend); }

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

// We should be able to nest calls to show() via the callback. ImGUI causes headaches here
TEST_F(PolyscopeTest, NestedShow) {

  auto showCallback = [&]() { polyscope::show(3); };
  polyscope::state::userCallback = showCallback;
  polyscope::show(3);

  polyscope::state::userCallback = nullptr;
}


// ============================================================
// =============== Point cloud tests
// ============================================================

std::vector<glm::vec3> getPoints() {
  std::vector<glm::vec3> points;

  // clang-format off
  points = {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
    {0, 0, 0},
  };

  return points;
};

polyscope::PointCloud* registerPointCloud(std::string name = "test1") {
  std::vector<glm::vec3> points = getPoints();
  return polyscope::registerPointCloud(name, points);
}


TEST_F(PolyscopeTest, ShowPointCloud) {
  auto psPoints = registerPointCloud();

  polyscope::show(3);
  EXPECT_TRUE(polyscope::hasPointCloud("test1"));
  EXPECT_FALSE(polyscope::hasPointCloud("test2"));
  polyscope::removeAllStructures();
  EXPECT_FALSE(polyscope::hasPointCloud("test1"));
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

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, PointCloudPick) {
  auto psPoints = registerPointCloud();

  // Don't bother trying to actually click on anything, but make sure this doesn't crash
  polyscope::pick::evaluatePickQuery(77, 88);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, PointCloudColor) {
  auto psPoints = registerPointCloud();
  std::vector<glm::vec3> vColors(psPoints->nPoints(), glm::vec3{.2, .3, .4});
  auto q1 = psPoints->addColorQuantity("vcolor", vColors);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, PointCloudParam) {
  auto psPoints = registerPointCloud();
  std::vector<glm::vec2> param(psPoints->nPoints(), glm::vec2{.2, .3});

  auto q1 = psPoints->addParameterizationQuantity("param", param);
  q1->setEnabled(true);
  polyscope::show(3);

  auto q2 = psPoints->addLocalParameterizationQuantity("local param", param);
  q2->setEnabled(true);
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, PointCloudScalar) {
  auto psPoints = registerPointCloud();
  std::vector<double> vScalar(psPoints->nPoints(), 7.);
  auto q1 = psPoints->addScalarQuantity("vScalar", vScalar);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, PointCloudVector) {
  auto psPoints = registerPointCloud();
  std::vector<glm::vec3> vals(psPoints->nPoints(), {1., 2., 3.});
  auto q1 = psPoints->addVectorQuantity("vals", vals);
  q1->setEnabled(true);
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

  psPoints->setPointRadiusQuantity("vScalar2");
  polyscope::show(3);

  psPoints->setPointRadiusQuantity("vScalar2", false); // no autoscaling
  polyscope::show(3);

  psPoints->clearPointRadiusQuantity();
  polyscope::show(3);

  polyscope::removeAllStructures();
}

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

  return std::tuple<std::vector<glm::vec3>, std::vector<std::vector<size_t>>>{points, faces};
};

polyscope::SurfaceMesh* registerTriangleMesh(std::string name = "test1") {
  std::vector<glm::vec3> points;
  std::vector<std::vector<size_t>> faces;
  std::tie(points, faces) = getTriangleMesh();
  return polyscope::registerSurfaceMesh(name, points, faces);
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

  // Material
  psMesh->setMaterial("wax");
  EXPECT_EQ(psMesh->getMaterial(), "wax");
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshPick) {
  auto psMesh = registerTriangleMesh();

  // Don't bother trying to actually click on anything, but make sure this doesn't crash
  polyscope::pick::evaluatePickQuery(77, 88);

  // Do it again with edges enabled
  psMesh->setEdgeWidth(1.0);
  polyscope::pick::evaluatePickQuery(77, 88);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshBackface) {
  auto psMesh = registerTriangleMesh();
  
  // Same appearance
  psMesh->setBackFacePolicy(polyscope::BackFacePolicy::Identical);
  EXPECT_EQ(psMesh->getBackFacePolicy(), polyscope::BackFacePolicy::Identical);
  polyscope::show(3);

  // Different appearance
  psMesh->setBackFacePolicy(polyscope::BackFacePolicy::Different);
  EXPECT_EQ(psMesh->getBackFacePolicy(), polyscope::BackFacePolicy::Different);
  polyscope::show(3);

  // Cull backfacing
  psMesh->setBackFacePolicy(polyscope::BackFacePolicy::Cull);
  EXPECT_EQ(psMesh->getBackFacePolicy(), polyscope::BackFacePolicy::Cull);
  polyscope::show(3);

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
  q1->setStyle(polyscope::ParamVizStyle::CHECKER);
  polyscope::show(3);
  q1->setStyle(polyscope::ParamVizStyle::GRID);
  polyscope::show(3);
  q1->setStyle(polyscope::ParamVizStyle::LOCAL_CHECK);
  polyscope::show(3);
  q1->setStyle(polyscope::ParamVizStyle::LOCAL_RAD);
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
  q1->setStyle(polyscope::ParamVizStyle::CHECKER);
  polyscope::show(3);
  q1->setStyle(polyscope::ParamVizStyle::GRID);
  polyscope::show(3);
  q1->setStyle(polyscope::ParamVizStyle::LOCAL_CHECK);
  polyscope::show(3);
  q1->setStyle(polyscope::ParamVizStyle::LOCAL_RAD);
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
  std::vector<glm::vec3> basisX(psMesh->nVertices(), {1., 2., 3.});
  psMesh->setVertexTangentBasisX(basisX);
  std::vector<glm::vec2> vals(psMesh->nVertices(), {1., 2.});
  auto q1 = psMesh->addVertexIntrinsicVectorQuantity("param", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshFaceIntrinsic) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> basisX(psMesh->nFaces(), {1., 2., 3.});
  psMesh->setFaceTangentBasisX(basisX);
  std::vector<glm::vec2> vals(psMesh->nFaces(), {1., 2.});
  auto q1 = psMesh->addFaceIntrinsicVectorQuantity("param", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshOneForm) {
  auto psMesh = registerTriangleMesh();
  // std::vector<glm::vec3> basisX(psMesh->nVertices(), {1., 2., 3.});
  // psMesh->setVertexTangentBasisX(basisX);
  std::vector<double> vals(psMesh->nEdges(), 3.);
  std::vector<char> orients(psMesh->nEdges(), true);
  auto q1 = psMesh->addOneFormIntrinsicVectorQuantity("one form vecs", vals, orients);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshVertexIntrinsicRibbon) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> basisX(psMesh->nVertices(), {1., 2., 3.});
  psMesh->setVertexTangentBasisX(basisX);
  std::vector<glm::vec3> basisXF(psMesh->nFaces(), {1., 2., 3.});
  psMesh->setFaceTangentBasisX(basisXF);
  std::vector<glm::vec2> vals(psMesh->nVertices(), {1., 2.});
  auto q1 = psMesh->addVertexIntrinsicVectorQuantity("param", vals);
  q1->setEnabled(true);
  q1->setRibbonEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshFaceIntrinsicRibbon) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> basisX(psMesh->nFaces(), {1., 2., 3.});
  psMesh->setFaceTangentBasisX(basisX);
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

TEST_F(PolyscopeTest, SurfaceMeshSurfaceGraph) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> nodes = {
      {1., 2., 3.},
      {3., 4., 5.},
      {5., 6., 7.},
  };
  std::vector<std::array<size_t, 2>> edges = {{0, 1}, {1, 2}, {2, 0}};
  auto q1 = psMesh->addSurfaceGraphQuantity("vals", nodes, edges);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshSurfaceGraphPath) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> nodes = {
      {1., 2., 3.},
      {3., 4., 5.},
      {5., 6., 7.},
  };
  auto q1 = psMesh->addSurfaceGraphQuantity("vals", std::vector<std::vector<glm::vec3>>({nodes, nodes}));
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}


// ============================================================
// =============== Curve network tests
// ============================================================

std::tuple<std::vector<glm::vec3>, std::vector<std::array<size_t, 2>>> getCurveNetwork() {
  std::vector<glm::vec3> points;
  std::vector<std::array<size_t, 2>> edges;

  // clang-format off
  points = {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
    {0, 0, 0},
  };

  edges = {
    {1, 3},
    {3, 0},
    {1, 0},
    {0, 2}
   };
  // clang-format on

  return std::tuple<std::vector<glm::vec3>, std::vector<std::array<size_t, 2>>>{points, edges};
};

polyscope::CurveNetwork* registerCurveNetwork(std::string name = "test1") {
  std::vector<glm::vec3> points;
  std::vector<std::array<size_t, 2>> edges;
  std::tie(points, edges) = getCurveNetwork();
  return polyscope::registerCurveNetwork(name, points, edges);
}


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
  polyscope::pick::evaluatePickQuery(77, 88);

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

TEST_F(PolyscopeTest, CurveNetworkScalarEdge) {
  auto psCurve = registerCurveNetwork();
  std::vector<double> eScalar(psCurve->nEdges(), 9.);
  auto q3 = psCurve->addEdgeScalarQuantity("eScalar", eScalar);
  q3->setEnabled(true);
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

// ============================================================
// =============== Volume mesh tests
// ============================================================

std::tuple<std::vector<glm::vec3>, std::vector<std::array<int, 8>>> getVolumeMeshData() {
  // clang-format off
  std::vector<glm::vec3> combined_verts = {
    {0, 0, 0},
    {1, 0, 0},
    {1, 1, 0},
    {0, 1, 0},
    {0, 0, 1},
    {1, 0, 1},
    {1, 1, 1},
    {0, 1, 1},
    {1, 1, 1.5}
  };

  std::vector<std::array<int, 8>> combined_cells = {
    {0, 1, 2, 3, 4, 5, 6, 7},
    {7, 5, 6, 8, -1, -1, -1, -1},
  };
  // clang-format on

  return std::make_tuple(combined_verts, combined_cells);
};

TEST_F(PolyscopeTest, ShowVolumeMesh) {
  // clang-format off

  // Tets only
  std::vector<glm::vec3> tet_verts = {
    {0, 0, 0},
    {0, 0, 1},
    {0, 1, 0},
    {0, 1, 1},
  };
  std::vector<std::array<size_t, 4>> tet_cells = {
    {0,1,2,4}
  };
  polyscope::registerTetMesh("tet", tet_verts, tet_cells);
   

  // Hexes only
  std::vector<glm::vec3> hex_verts = {
    {0, 0, 0},
    {0, 0, 1},
    {0, 1, 0},
    {0, 1, 1},
    {1, 0, 0},
    {1, 0, 1},
    {1, 1, 0},
    {1, 1, 1},
  };
  std::vector<std::array<size_t, 8>> hex_cells = {
    {0,1,2,3,4,5,6,7},
  };
  polyscope::registerHexMesh("hex", hex_verts, hex_cells);


  // clang-format on

  // Mixed elements, separate arrays
  std::vector<glm::vec3> combined_verts;
  combined_verts.insert(combined_verts.end(), tet_verts.begin(), tet_verts.end());
  combined_verts.insert(combined_verts.end(), hex_verts.begin(), hex_verts.end());
  for (auto& hex : hex_cells) {
    for (size_t& i : hex) {
      i += tet_verts.size();
    }
  }
  polyscope::registerTetHexMesh("tet hex mix separate", combined_verts, tet_cells, hex_cells);


  // Mixed elements, shared array
  std::vector<std::array<int, 8>> combined_cells = {
      {0, 1, 3, 4, -1, -1, -1, -1},
      {4, 5, 6, 7, 8, 9, 10, 11},
  };
  polyscope::registerVolumeMesh("tet hex mix combined", combined_verts, combined_cells);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, VolumeMeshAppearance) {
  std::vector<glm::vec3> verts;
  std::vector<std::array<int, 8>> cells;
  std::tie(verts, cells) = getVolumeMeshData();
  polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);

  // Material
  psVol->setMaterial("wax");
  EXPECT_EQ(psVol->getMaterial(), "wax");
  polyscope::show(3);

  // Color of the mesh
  glm::vec3 color{0.5, 0.25, 0.25};
  psVol->setColor(color);
  EXPECT_EQ(color, psVol->getColor());

  // Color of the mesh inteiror
  glm::vec3 colorI{0.5, 0.25, 0.75};
  psVol->setInteriorColor(colorI);
  EXPECT_EQ(colorI, psVol->getInteriorColor());

  // Color of the mesh edges
  glm::vec3 colorE{0.5, 0.25, 0.5};
  psVol->setEdgeColor(colorE);
  EXPECT_EQ(colorE, psVol->getEdgeColor());
  polyscope::show(3);

  // Edge width
  psVol->setEdgeWidth(0.25);
  EXPECT_EQ(0.25, psVol->getEdgeWidth());

  // Transparency
  psVol->setTransparency(0.25);
  EXPECT_EQ(0.25, psVol->getTransparency());

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, VolumeMeshPick) {
  std::vector<glm::vec3> verts;
  std::vector<std::array<int, 8>> cells;
  std::tie(verts, cells) = getVolumeMeshData();
  polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);

  // Don't bother trying to actually click on anything, but make sure this doesn't crash
  polyscope::pick::evaluatePickQuery(77, 88);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, VolumeMeshColorVertex) {
  std::vector<glm::vec3> verts;
  std::vector<std::array<int, 8>> cells;
  std::tie(verts, cells) = getVolumeMeshData();
  polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);
  
  std::vector<glm::vec3> vColors(verts.size(), glm::vec3{.2, .3, .4});
  auto q1 = psVol->addVertexColorQuantity("vcolor", vColors);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, VolumeMeshColorCell) {
  std::vector<glm::vec3> verts;
  std::vector<std::array<int, 8>> cells;
  std::tie(verts, cells) = getVolumeMeshData();
  polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);
  
  std::vector<glm::vec3> cColors(cells.size(), glm::vec3{.2, .3, .4});
  auto q1 = psVol->addCellColorQuantity("ccolor", cColors);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, VolumeMeshScalarVertex) {
  std::vector<glm::vec3> verts;
  std::vector<std::array<int, 8>> cells;
  std::tie(verts, cells) = getVolumeMeshData();
  polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);
  
  std::vector<float> vals(verts.size(), 0.44);
  auto q1 = psVol->addVertexScalarQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, VolumeMeshScalarCell) {
  std::vector<glm::vec3> verts;
  std::vector<std::array<int, 8>> cells;
  std::tie(verts, cells) = getVolumeMeshData();
  polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);
  
  std::vector<float> vals(cells.size(), 0.44);
  auto q1 = psVol->addCellScalarQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, VolumeMeshVertexVector) {
  std::vector<glm::vec3> verts;
  std::vector<std::array<int, 8>> cells;
  std::tie(verts, cells) = getVolumeMeshData();
  polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);
  
  std::vector<glm::vec3> vals(verts.size(), {1., 2., 3.});
  auto q1 = psVol->addVertexVectorQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, VolumeMeshCellVector) {
  std::vector<glm::vec3> verts;
  std::vector<std::array<int, 8>> cells;
  std::tie(verts, cells) = getVolumeMeshData();
  polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);
  
  std::vector<glm::vec3> vals(cells.size(), {1., 2., 3.});
  auto q1 = psVol->addCellVectorQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}


// ============================================================
// =============== Combo test
// ============================================================


// Register a handful of quantities / structures, then call refresh
TEST_F(PolyscopeTest, RefreshMultiTest) {

  { // Surface mesh
    auto psMesh = registerTriangleMesh();
    std::vector<double> vScalar(psMesh->nVertices(), 7.);
    auto q1 = psMesh->addVertexDistanceQuantity("distance", vScalar);
  }

  { // Point cloud
    auto psPoints = registerPointCloud();
    std::vector<double> vScalar(psPoints->nPoints(), 7.);
    auto q2 = psPoints->addScalarQuantity("vScalar", vScalar);
    q2->setEnabled(true);
  }

  { // Curve network
    auto psCurve = registerCurveNetwork();
    std::vector<glm::vec3> vals(psCurve->nEdges(), {1., 2., 3.});
    auto q3 = psCurve->addEdgeVectorQuantity("vals", vals);
    q3->setEnabled(true);
  }

  polyscope::show(3);

  polyscope::refresh();
  polyscope::show(3);

  polyscope::removeAllStructures();
}

// Cycle through the transparency optins
TEST_F(PolyscopeTest, TransparencyTest) {

  { // Surface mesh
    auto psMesh = registerTriangleMesh();
    std::vector<double> vScalar(psMesh->nVertices(), 7.);
    auto q1 = psMesh->addVertexDistanceQuantity("distance", vScalar);
  }

  { // Point cloud
    auto psPoints = registerPointCloud();
    std::vector<double> vScalar(psPoints->nPoints(), 7.);
    auto q2 = psPoints->addScalarQuantity("vScalar", vScalar);
    q2->setEnabled(true);
  }

  { // Curve network
    auto psCurve = registerCurveNetwork();
    std::vector<glm::vec3> vals(psCurve->nEdges(), {1., 2., 3.});
    auto q3 = psCurve->addEdgeVectorQuantity("vals", vals);
    q3->setEnabled(true);
  }

  polyscope::show(3);

  polyscope::options::transparencyMode = polyscope::TransparencyMode::Simple;
  polyscope::show(3);

  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::show(3);

  polyscope::removeAllStructures();
}

// Do some slice plane stuff
TEST_F(PolyscopeTest, SlicePlaneTest) {

  // Surface mesh
  auto psMesh = registerTriangleMesh();
  std::vector<double> vScalar(psMesh->nVertices(), 7.);
  auto q1 = psMesh->addVertexDistanceQuantity("distance", vScalar);

  { // Point cloud
    auto psPoints = registerPointCloud();
    std::vector<double> vScalar(psPoints->nPoints(), 7.);
    auto q2 = psPoints->addScalarQuantity("vScalar", vScalar);
    q2->setEnabled(true);
  }

  { // Curve network
    auto psCurve = registerCurveNetwork();
    std::vector<glm::vec3> vals(psCurve->nEdges(), {1., 2., 3.});
    auto q3 = psCurve->addEdgeVectorQuantity("vals", vals);
    q3->setEnabled(true);
  }

  { // Volume mesh
    std::vector<glm::vec3> verts;
    std::vector<std::array<int, 8>> cells;
    std::tie(verts, cells) = getVolumeMeshData();
    polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);

    polyscope::VolumeMesh* psVol2 = polyscope::registerVolumeMesh("vol cull whole", verts, cells);
    psVol2->setCullWholeElements(true);
  }

  polyscope::show(3);

  // render with one slice plane
  polyscope::addSceneSlicePlane();
  polyscope::show(3);

  // add another and rotate it
  polyscope::SlicePlane* p = polyscope::addSceneSlicePlane();
  p->setTransform(glm::translate(p->getTransform(), glm::vec3{-1., 0., 0.}));
  polyscope::show(3);

  // test removal
  polyscope::removeLastSceneSlicePlane();
  polyscope::show(3);

  // make one structure ignore the plane
  psMesh->setIgnoreSlicePlane(polyscope::state::slicePlanes[0]->name, true);
  polyscope::show(3);

  // remove the last plane so we don't leave it around for future tests
  polyscope::removeLastSceneSlicePlane();

  polyscope::removeAllStructures();
}
