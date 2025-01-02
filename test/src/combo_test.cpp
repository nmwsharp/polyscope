// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"


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

  // Point cloud
  auto psPoints = registerPointCloud();
  psPoints->setPointRenderMode(polyscope::PointRenderMode::Sphere);
  psPoints->setCullWholeElements(true);
  std::vector<double> vScalarP(psPoints->nPoints(), 7.);
  auto q2 = psPoints->addScalarQuantity("vScalar", vScalarP);
  q2->setEnabled(true);


  {
    // Curve network
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

  // try a few variations of point cloud settings
  psPoints->setCullWholeElements(false);
  polyscope::show(3);
  psPoints->setCullWholeElements(true);
  psPoints->setPointRenderMode(polyscope::PointRenderMode::Quad);
  polyscope::show(3);
  psPoints->setCullWholeElements(false);
  polyscope::show(3);

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

// Register a handful of quantities / structures, then call refresh
TEST_F(PolyscopeTest, OrthoViewTest) {

  // Add some stuff

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

  // Enable the orthographic view
  polyscope::view::projectionMode = polyscope::ProjectionMode::Orthographic;
  polyscope::show(3);

  // Go back to default perspective
  polyscope::view::projectionMode = polyscope::ProjectionMode::Perspective;

  polyscope::removeAllStructures();
}
