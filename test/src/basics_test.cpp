// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"

#include "polyscope/curve_network.h"
#include "polyscope/pick.h"
#include "polyscope/point_cloud.h"
#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/types.h"
#include "polyscope/volume_mesh.h"

#include "gtest/gtest.h"

#include <array>
#include <iostream>
#include <list>
#include <string>
#include <vector>


// ============================================================
// =============== Basic tests
// ============================================================


// Show the gui. Note that the pre-suite script calls Polyscope::init() before
TEST_F(PolyscopeTest, InitializeAndShow) { polyscope::show(3); }

TEST_F(PolyscopeTest, FrameTick) {
  for (int i = 0; i < 5; i++) {
    polyscope::frameTick();
  }
}

// We should be able to nest calls to show() via the callback. ImGUI causes headaches here
TEST_F(PolyscopeTest, NestedShow) {

  auto showCallback = [&]() { polyscope::show(3); };
  polyscope::state::userCallback = showCallback;
  polyscope::show(3);

  polyscope::state::userCallback = nullptr;
}


// Make sure that creating an empty buffer does not throw errors
TEST_F(PolyscopeTest, EmptyBuffer) {


  std::vector<glm::vec3> empty_points;
  polyscope::PointCloud* psPoints = polyscope::registerPointCloud("empty cloud", empty_points);
  polyscope::show(3);

  std::vector<std::array<uint32_t, 2>> empty_edges;
  polyscope::CurveNetwork* psNet = polyscope::registerCurveNetwork("empty curve", empty_points, empty_edges);
  polyscope::show(3);

  polyscope::removeAllStructures();
}


// ============================================================
// =============== Ground plane tests
// ============================================================

TEST_F(PolyscopeTest, GroundPlaneTest) {

  // Add a structure and cycle through the ground plane options
  auto psMesh = registerTriangleMesh();

  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::None;
  polyscope::refresh();
  polyscope::show(3);

  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::Tile;
  polyscope::refresh();
  polyscope::show(3);

  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::TileReflection;
  polyscope::refresh();
  polyscope::show(3);

  polyscope::options::groundPlaneMode = polyscope::GroundPlaneMode::ShadowOnly;
  polyscope::refresh();
  polyscope::show(3);

  polyscope::removeAllStructures();
}
