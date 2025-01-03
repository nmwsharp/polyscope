// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"

// ============================================================
// =============== Surface mesh tests
// ============================================================


TEST_F(PolyscopeTest, ShowSurfaceMesh) {
  auto psMesh = registerTriangleMesh();
  EXPECT_TRUE(polyscope::hasSurfaceMesh("test1"));

  // Make sure we actually added the mesh
  polyscope::show(3);
  EXPECT_TRUE(polyscope::hasSurfaceMesh("test1"));
  EXPECT_FALSE(polyscope::hasSurfaceMesh("test2"));
  polyscope::removeAllStructures();
  EXPECT_FALSE(polyscope::hasSurfaceMesh("test1"));
}

TEST_F(PolyscopeTest, SurfaceMesh2D) {
  // test meshes with 2D vertex positions

  std::vector<glm::vec2> points;
  std::vector<std::vector<size_t>> faces;

  // clang-format off
  points = {
    {1, 0},
    {0, 1},
    {0, 0},
    {0, 0},
  };

  faces = {
    {1, 3, 2},
    {3, 1, 0},
    {2, 0, 1},
    {0, 2, 3}
   };
  // clang-format on

  polyscope::registerSurfaceMesh2D("mesh2d", points, faces);

  // Make sure we actually added the mesh
  polyscope::show(3);
  EXPECT_TRUE(polyscope::hasSurfaceMesh("mesh2d"));

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshPolygon) {
  // meshes with polygonal (greater-than-triangular) faces
  std::vector<glm::vec2> points;
  std::vector<std::vector<size_t>> faces;

  // clang-format off
  points = {
    {1, 0},
    {0, 1},
    {0, 0},
    {0, 0},
  };

  faces = {
    {1, 3, 2, 0},
    {3, 1, 0},
    {2, 0, 1, 3},
    {0, 2, 3}
   };
  // clang-format on

  polyscope::registerSurfaceMesh2D("mesh poly", points, faces);

  // Make sure we actually added the mesh
  polyscope::show(3);
  EXPECT_TRUE(polyscope::hasSurfaceMesh("mesh poly"));

  polyscope::removeAllStructures();
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

  psMesh->setShadeStyle(polyscope::MeshShadeStyle::TriFlat);
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

TEST_F(PolyscopeTest, SurfaceMeshMark) {
  auto psMesh = registerTriangleMesh();

  // edges
  size_t nEdges = 6;
  std::vector<size_t> ePerm = {5, 3, 1, 2, 4, 0};
  psMesh->setEdgePermutation(ePerm);
  psMesh->markEdgesAsUsed();
  polyscope::show(3);

  // halfedges
  std::vector<size_t> hePerm;
  for (size_t i = 0; i < psMesh->nCorners(); i++) {
    hePerm.push_back(5 + i);
  }
  psMesh->setHalfedgePermutation(hePerm);
  psMesh->markHalfedgesAsUsed();
  polyscope::show(3);

  // corners
  // (permutation is not required for this one)
  psMesh->markCornersAsUsed();
  polyscope::show(3);

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
  psMesh->setBackFaceColor(glm::vec3(1.f, 0.f, 0.f));
  EXPECT_EQ(psMesh->getBackFaceColor(), glm::vec3(1.f, 0.f, 0.f));
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

TEST_F(PolyscopeTest, SurfaceMeshColorTexture) {
  auto psMesh = registerTriangleMesh();

  std::vector<glm::vec2> vals(psMesh->nCorners(), {1., 2.});
  auto qParam = psMesh->addParameterizationQuantity("param", vals);

  size_t dimX = 10;
  size_t dimY = 15;
  std::vector<glm::vec3> colorsTex(dimX * dimY, glm::vec3{.2, .3, .4});
  polyscope::SurfaceTextureColorQuantity* qColor =
      psMesh->addTextureColorQuantity("tColor", *qParam, dimX, dimY, colorsTex, polyscope::ImageOrigin::UpperLeft);
  qColor->setEnabled(true);

  polyscope::show(3);

  // test options
  qColor->setFilterMode(polyscope::FilterMode::Nearest);
  polyscope::show(3);

  // make sure the by-name adder also works
  polyscope::SurfaceTextureColorQuantity* qColor2 =
      psMesh->addTextureColorQuantity("tColor2", "param", dimX, dimY, colorsTex, polyscope::ImageOrigin::UpperLeft);

  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarVertex) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> vScalar(psMesh->nVertices(), 7.);
  auto q1 = psMesh->addVertexScalarQuantity("vScalar", vScalar);
  q1->setEnabled(true);
  polyscope::show(3);
  
  // try some options
  q1->setIsolinesEnabled(true);
  polyscope::show(3);
  q1->setIsolineStyle(polyscope::IsolineStyle::Contour);
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarCategoricalVertex) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> vScalar(psMesh->nVertices(), 7.);
  auto q1 = psMesh->addVertexScalarQuantity("vScalar", vScalar, polyscope::DataType::CATEGORICAL);
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

TEST_F(PolyscopeTest, SurfaceMeshScalarCategoricalFace) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> fScalar(psMesh->nFaces(), 8.);
  auto q2 = psMesh->addFaceScalarQuantity("fScalar", fScalar, polyscope::DataType::CATEGORICAL);
  q2->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarEdge) {
  auto psMesh = registerTriangleMesh();
  size_t nEdges = 6;
  std::vector<double> eScalar(nEdges, 9.);
  std::vector<size_t> ePerm = {5, 3, 1, 2, 4, 0};
  psMesh->setEdgePermutation(ePerm);
  auto q3 = psMesh->addEdgeScalarQuantity("eScalar", eScalar);
  q3->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarCategoricalEdge) {
  auto psMesh = registerTriangleMesh();
  size_t nEdges = 6;
  std::vector<double> eScalar(nEdges, 9.);
  std::vector<size_t> ePerm = {5, 3, 1, 2, 4, 0};
  psMesh->setEdgePermutation(ePerm);
  auto q3 = psMesh->addEdgeScalarQuantity("eScalar", eScalar, polyscope::DataType::CATEGORICAL);
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

TEST_F(PolyscopeTest, SurfaceMeshScalarCategoricalHalfedge) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> heScalar(psMesh->nHalfedges(), 10.);
  auto q4 = psMesh->addHalfedgeScalarQuantity("heScalar", heScalar, polyscope::DataType::CATEGORICAL);
  q4->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarHalfedgePerm) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> heScalar(5 + psMesh->nHalfedges(), 10.);
  std::vector<size_t> hePerm;
  for (size_t i = 0; i < psMesh->nCorners(); i++) {
    hePerm.push_back(5 + i);
  }
  psMesh->setHalfedgePermutation(hePerm);
  auto q4 = psMesh->addHalfedgeScalarQuantity("heScalar", heScalar);
  q4->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarCorner) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> cornerScalar(psMesh->nCorners(), 10.);
  auto q4 = psMesh->addCornerScalarQuantity("cornerScalar", cornerScalar);
  q4->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarCategoricalCorner) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> cornerScalar(psMesh->nCorners(), 10.);
  auto q4 = psMesh->addCornerScalarQuantity("cornerScalar", cornerScalar, polyscope::DataType::CATEGORICAL);
  q4->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarCornerPerm) {
  auto psMesh = registerTriangleMesh();
  std::vector<double> cornerScalar(5 + psMesh->nCorners(), 10.);
  std::vector<size_t> cPerm;
  for (size_t i = 0; i < psMesh->nCorners(); i++) {
    cPerm.push_back(5 + i);
  }
  psMesh->setCornerPermutation(cPerm);
  auto q4 = psMesh->addCornerScalarQuantity("cornerScalar", cornerScalar);
  q4->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarTexture) {
  auto psMesh = registerTriangleMesh();

  std::vector<glm::vec2> vals(psMesh->nCorners(), {1., 2.});
  auto qParam = psMesh->addParameterizationQuantity("param", vals);

  size_t dimX = 10;
  size_t dimY = 15;
  std::vector<float> valuesTex(dimX * dimY, 0.77);
  polyscope::SurfaceTextureScalarQuantity* qScalar =
      psMesh->addTextureScalarQuantity("tScalar", *qParam, dimX, dimY, valuesTex, polyscope::ImageOrigin::UpperLeft);
  qScalar->setEnabled(true);
  polyscope::show(3);

  // test options
  qScalar->setFilterMode(polyscope::FilterMode::Nearest);
  polyscope::show(3);

  // make sure the by-name adder also works
  polyscope::SurfaceTextureScalarQuantity* qScalar2 =
      psMesh->addTextureScalarQuantity("tScalar2", "param", dimX, dimY, valuesTex, polyscope::ImageOrigin::UpperLeft);

  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarCategoricalTexture) {
  auto psMesh = registerTriangleMesh();

  std::vector<glm::vec2> vals(psMesh->nCorners(), {1., 2.});
  auto qParam = psMesh->addParameterizationQuantity("param", vals);

  size_t dimX = 10;
  size_t dimY = 15;
  std::vector<float> valuesTex(dimX * dimY, 0.77);
  polyscope::SurfaceTextureScalarQuantity* qScalar = psMesh->addTextureScalarQuantity(
      "tScalar", *qParam, dimX, dimY, valuesTex, polyscope::ImageOrigin::UpperLeft, polyscope::DataType::CATEGORICAL);
  qScalar->setEnabled(true);
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshScalarTransparency) {

  auto psMesh = registerTriangleMesh();

  { // vertex quantity
    std::vector<double> vScalar(psMesh->nVertices(), 7.);
    auto qVert = psMesh->addVertexScalarQuantity("vScalar", vScalar);
    psMesh->setTransparencyQuantity(qVert);
    polyscope::show(3);
  }

  // make sure clear works
  psMesh->clearTransparencyQuantity();
  polyscope::show(3);

  { // face quantity
    std::vector<double> fScalar(psMesh->nFaces(), 8.);
    auto qFace = psMesh->addFaceScalarQuantity("fScalar", fScalar);
    psMesh->setTransparencyQuantity(qFace);
    polyscope::show(3);
  }

  { // corner quantity
    std::vector<double> cornerScalar(psMesh->nCorners(), 10.);
    auto qCorner = psMesh->addCornerScalarQuantity("cornerScalar", cornerScalar);
    psMesh->setTransparencyQuantity(qCorner);
    polyscope::show(3);
  }

  // edges, halfedges, and textures are currently not supported

  // Change transparency settings
  polyscope::options::transparencyMode = polyscope::TransparencyMode::Simple;
  polyscope::show(3);
  polyscope::options::transparencyMode = polyscope::TransparencyMode::None;
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

  // set islands
  std::vector<int32_t> islandLabels(psMesh->nFaces(), 0);
  q1->setIslandLabels(islandLabels);
  q1->setStyle(polyscope::ParamVizStyle::CHECKER_ISLANDS);
  polyscope::show(3);


  // create the curve network
  q1->createCurveNetworkFromSeams();
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
  auto q1 = psMesh->addVertexVectorQuantity("vecs", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshFaceVector) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> vals(psMesh->nFaces(), {1., 2., 3.});
  auto q1 = psMesh->addFaceVectorQuantity("vecs", vals);
  q1->setEnabled(true);
  // symmetric case
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshVertexTangent) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> basisX(psMesh->nVertices(), {1., 2., 3.});
  std::vector<glm::vec3> basisY(psMesh->nVertices(), {1., 2., 3.});
  std::vector<glm::vec2> vals(psMesh->nVertices(), {1., 2.});
  auto q1 = psMesh->addVertexTangentVectorQuantity("vecs", vals, basisX, basisY);
  q1->setEnabled(true);
  polyscope::show(3);
  // symmetric case
  auto q2 = psMesh->addVertexTangentVectorQuantity("sym vecs", vals, basisX, basisY, 4);
  q2->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshFaceTangent) {
  auto psMesh = registerTriangleMesh();
  std::vector<glm::vec3> basisX(psMesh->nFaces(), {1., 2., 3.});
  std::vector<glm::vec3> basisY(psMesh->nFaces(), {1., 2., 3.});
  std::vector<glm::vec2> vals(psMesh->nFaces(), {1., 2.});
  auto q1 = psMesh->addFaceTangentVectorQuantity("vecs", vals, basisX, basisY);
  q1->setEnabled(true);
  polyscope::show(3);
  // symmetric case
  auto q2 = psMesh->addFaceTangentVectorQuantity("sym vecs", vals, basisX, basisY, 4);
  q2->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SurfaceMeshOneForm) {
  auto psMesh = registerTriangleMesh();
  size_t nEdges = 6;
  std::vector<double> vals(nEdges, 3.);
  std::vector<char> orients(nEdges, true);
  std::vector<size_t> ePerm = {5, 3, 1, 2, 4, 0};
  psMesh->setEdgePermutation(ePerm);
  auto q1 = psMesh->addOneFormTangentVectorQuantity("one form vecs", vals, orients);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();
}


// ============================================================
// =============== Simple Surface Mesh
// ============================================================


TEST_F(PolyscopeTest, ShowSimpleTriangleMesh) {
  auto psMesh = registerSimpleTriangleMesh();
  EXPECT_TRUE(polyscope::hasSimpleTriangleMesh("test1"));

  // Make sure we actually added the mesh
  polyscope::show(3);
  EXPECT_TRUE(polyscope::hasSimpleTriangleMesh("test1"));
  EXPECT_FALSE(polyscope::hasSimpleTriangleMesh("test2"));
  polyscope::removeAllStructures();
  EXPECT_FALSE(polyscope::hasSimpleTriangleMesh("test1"));
}

TEST_F(PolyscopeTest, SimpleTriangleMeshAppearance) {
  auto psMesh = registerSimpleTriangleMesh();

  // Material
  psMesh->setMaterial("wax");
  EXPECT_EQ(psMesh->getMaterial(), "wax");
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SimpleTriangleMeshPick) {
  auto psMesh = registerSimpleTriangleMesh();

  // Don't bother trying to actually click on anything, but make sure this doesn't crash
  polyscope::pick::evaluatePickQuery(77, 88);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, SimpleTriangleMeshBackface) {
  auto psMesh = registerSimpleTriangleMesh();

  // Same appearance
  psMesh->setBackFacePolicy(polyscope::BackFacePolicy::Identical);
  EXPECT_EQ(psMesh->getBackFacePolicy(), polyscope::BackFacePolicy::Identical);
  polyscope::show(3);

  // Different appearance
  psMesh->setBackFacePolicy(polyscope::BackFacePolicy::Different);
  EXPECT_EQ(psMesh->getBackFacePolicy(), polyscope::BackFacePolicy::Different);
  psMesh->setBackFaceColor(glm::vec3(1.f, 0.f, 0.f));
  EXPECT_EQ(psMesh->getBackFaceColor(), glm::vec3(1.f, 0.f, 0.f));
  polyscope::show(3);

  // Cull backfacing
  psMesh->setBackFacePolicy(polyscope::BackFacePolicy::Cull);
  EXPECT_EQ(psMesh->getBackFacePolicy(), polyscope::BackFacePolicy::Cull);
  polyscope::show(3);

  polyscope::removeAllStructures();
}


TEST_F(PolyscopeTest, SimpleTriangleMesUpdate) {
  auto psMesh = registerSimpleTriangleMesh();
  polyscope::show(3); // make sure everything is populated

  // update just the locations
  psMesh->updateVertices(std::vector<glm::vec3>(4));
  polyscope::show(3);

  // update the locations and faces
  psMesh->update(std::vector<glm::vec3>(4), std::vector<glm::uvec3>(4, glm::uvec3(0, 1, 2)));
  polyscope::show(3);

  // do a bunch of resizing
  psMesh->update(std::vector<glm::vec3>(12), std::vector<glm::uvec3>(4, glm::uvec3(0, 1, 2)));
  polyscope::show(3);

  psMesh->update(std::vector<glm::vec3>(3), std::vector<glm::uvec3>(4, glm::uvec3(0, 1, 2)));
  polyscope::show(3);

  psMesh->update(std::vector<glm::vec3>(3), std::vector<glm::uvec3>(14, glm::uvec3(0, 1, 2)));
  polyscope::show(3);

  psMesh->update(std::vector<glm::vec3>(3), std::vector<glm::uvec3>(1, glm::uvec3(0, 1, 2)));
  polyscope::show(3);

  polyscope::removeAllStructures();
}
