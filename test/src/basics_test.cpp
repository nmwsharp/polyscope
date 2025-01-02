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

TEST_F(PolyscopeTest, FrameTickWithImgui) {

  auto showCallback = [&]() { ImGui::Button("do something"); };
  polyscope::state::userCallback = showCallback;

  for (int i = 0; i < 5; i++) {
    polyscope::frameTick();
  }

  polyscope::state::userCallback = nullptr;
}

// We should be able to nest calls to show() via the callback. ImGUI causes headaches here
TEST_F(PolyscopeTest, NestedShow) {

  auto showCallback = [&]() { polyscope::show(3); };
  polyscope::state::userCallback = showCallback;
  polyscope::show(3);

  polyscope::state::userCallback = nullptr;
}

TEST_F(PolyscopeTest, NestedShowWithFrameTick) {

  auto showCallback = [&]() { polyscope::show(3); };
  polyscope::state::userCallback = showCallback;

  for (int i = 0; i < 3; i++) {
    polyscope::frameTick();
  }

  polyscope::state::userCallback = nullptr;
}

TEST_F(PolyscopeTest, Unshow) {

  int32_t count = 0;
  auto showCallback = [&]() {
    if (count > 1) {
      polyscope::unshow();
    }
    count++;
  };
  polyscope::state::userCallback = showCallback;
  polyscope::show(10);

  EXPECT_LT(count, 4);

  polyscope::state::userCallback = nullptr;
}

TEST_F(PolyscopeTest, ShutdownAndReinitialize) {
  polyscope::shutdown();
  SetUpTestSuite();
  polyscope::show(3);

  // do it twice -- we've had some bugs where the first shutdown doesn't clean up properly
  polyscope::shutdown();
  SetUpTestSuite();
  polyscope::show(3);
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

TEST_F(PolyscopeTest, WindowProperties) {

  // set/get window size
  polyscope::view::setWindowSize(300, 400);
  int32_t w, h;
  std::tie(w, h) = polyscope::view::getWindowSize();
  EXPECT_EQ(w, 300);
  EXPECT_EQ(h, 400);

  // get buffer size
  // (hard to say what this should be, given hi-dpi etc)
  std::tie(w, h) = polyscope::view::getBufferSize();

  // resizable
  polyscope::view::setWindowResizable(false);
  EXPECT_FALSE(polyscope::view::getWindowResizable());

  polyscope::show(3);
}

TEST_F(PolyscopeTest, Screenshot) { polyscope::screenshot("test_screeshot.png"); }

TEST_F(PolyscopeTest, ScreenshotBuffer) {
  std::vector<unsigned char> buff = polyscope::screenshotToBuffer();
  EXPECT_EQ(buff.size(), polyscope::view::bufferWidth * polyscope::view::bufferHeight * 4);

  std::vector<unsigned char> buff2 = polyscope::screenshotToBuffer(false);
  EXPECT_EQ(buff2.size(), polyscope::view::bufferWidth * polyscope::view::bufferHeight * 4);
}

// ============================================================
// =============== View and navigation
// ============================================================

TEST_F(PolyscopeTest, NavigationMode) {

  // Cycle through the navigation options

  polyscope::view::setNavigateStyle(polyscope::NavigateStyle::Turntable);
  polyscope::show(3);

  polyscope::view::setNavigateStyle(polyscope::NavigateStyle::Free);
  polyscope::show(3);

  polyscope::view::setNavigateStyle(polyscope::NavigateStyle::Planar);
  polyscope::show(3);

  polyscope::view::setNavigateStyle(polyscope::NavigateStyle::Arcball);
  polyscope::show(3);

  polyscope::view::setNavigateStyle(polyscope::NavigateStyle::None);
  polyscope::show(3);

  polyscope::view::setNavigateStyle(polyscope::NavigateStyle::FirstPerson);
  polyscope::show(3);

  polyscope::view::setNavigateStyle(polyscope::NavigateStyle::Turntable); // set back to usual default
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

  polyscope::options::groundPlaneHeightMode = polyscope::GroundPlaneHeightMode::Manual;
  polyscope::options::groundPlaneHeight = -0.3;
  polyscope::show(3);

  polyscope::options::groundPlaneHeightMode = polyscope::GroundPlaneHeightMode::Automatic;
  polyscope::show(3);

  polyscope::removeAllStructures();
}
