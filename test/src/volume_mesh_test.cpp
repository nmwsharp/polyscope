// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"

// ============================================================
// =============== Volume mesh tests
// ============================================================

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

TEST_F(PolyscopeTest, VolumeMeshUpdatePositions) {
  std::vector<glm::vec3> verts;
  std::vector<std::array<int, 8>> cells;
  std::tie(verts, cells) = getVolumeMeshData();
  polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);

  polyscope::show(3);

  psVol->updateVertexPositions(verts);

  polyscope::show(3);

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

TEST_F(PolyscopeTest, VolumeMeshInspect) {
  std::vector<glm::vec3> verts;
  std::vector<std::array<int, 8>> cells;
  std::tie(verts, cells) = getVolumeMeshData();
  polyscope::VolumeMesh* psVol = polyscope::registerVolumeMesh("vol", verts, cells);

  // plain old inspecting
  polyscope::SlicePlane* p = polyscope::addSceneSlicePlane();
  p->setVolumeMeshToInspect("vol");
  polyscope::show(3);

  // with a scalar quantity
  std::vector<float> vals(verts.size(), 0.44);
  auto q1 = psVol->addVertexScalarQuantity("vals", vals);
  q1->setEnabled(true);
  polyscope::show(3);
  polyscope::removeAllStructures();

  polyscope::removeLastSceneSlicePlane();
}
