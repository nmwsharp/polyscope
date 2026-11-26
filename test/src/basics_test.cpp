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
  int32_t target_w = 500;
  int32_t target_h = 300;
  polyscope::view::setWindowSize(target_w, target_h);
  int32_t w, h;
  std::tie(w, h) = polyscope::view::getWindowSize();
  EXPECT_EQ(w, target_w);
  EXPECT_EQ(h, target_h);

  polyscope::show(3);

  // get buffer size
  // (hard to say what this should be, given hi-dpi etc)
  int32_t buffer_w, buffer_h;
  std::tie(buffer_w, buffer_h) = polyscope::view::getBufferSize();

  float target_aspect = target_w / static_cast<float>(target_h);
  float buffer_aspect = buffer_w / static_cast<float>(buffer_h);
  EXPECT_NEAR(target_aspect, buffer_aspect, 0.01);


  // resizable
  polyscope::view::setWindowResizable(false);
  EXPECT_FALSE(polyscope::view::getWindowResizable());

  polyscope::show(3);
}

TEST_F(PolyscopeTest, Screenshot) {
  polyscope::screenshot("test_screeshot.png");
  polyscope::screenshot();

  polyscope::ScreenshotOptions opts;
  opts.includeUI = true;
  opts.transparentBackground = false;
  polyscope::screenshot(opts);
}

TEST_F(PolyscopeTest, ScreenshotBuffer) {
  std::vector<unsigned char> buff = polyscope::screenshotToBuffer();
  EXPECT_EQ(buff.size(), polyscope::view::bufferWidth * polyscope::view::bufferHeight * 4);

  std::vector<unsigned char> buff2 = polyscope::screenshotToBuffer(false);
  EXPECT_EQ(buff2.size(), polyscope::view::bufferWidth * polyscope::view::bufferHeight * 4);
}

TEST_F(PolyscopeTest, ImPlotBasic) {

  std::vector<float> xvals = {0., 2., 4., 8.};

  auto showCallback = [&]() {
    ImGui::Button("do something");
    if (ImPlot::BeginPlot("test plot")) {
      ImPlot::PlotLine("test line", &xvals.front(), xvals.size());
      ImPlot::EndPlot();
    }
  };
  polyscope::state::userCallback = showCallback;

  polyscope::show(3);

  for (int i = 0; i < 3; i++) {
    polyscope::frameTick();
  }

  polyscope::state::userCallback = nullptr;
}


TEST_F(PolyscopeTest, ImPlotScreenshot) {
  // test this because there is some context logic duplicated there

  std::vector<float> xvals = {0., 2., 4., 8.};

  auto showCallback = [&]() {
    ImGui::Button("do something");
    if (ImPlot::BeginPlot("test plot")) {
      ImPlot::PlotLine("test line", &xvals.front(), xvals.size());
      ImPlot::EndPlot();
    }
  };
  polyscope::state::userCallback = showCallback;

  polyscope::show(3);

  polyscope::ScreenshotOptions opts;
  opts.includeUI = true;
  opts.transparentBackground = false;
  polyscope::screenshot(opts);

  polyscope::show(3);

  polyscope::state::userCallback = nullptr;
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

TEST_F(PolyscopeTest, ViewportManagement) {
  auto psMesh = registerTriangleMesh();

  // 2x1 grid
  polyscope::view::createViewportGrid(2, 1);
  EXPECT_EQ(polyscope::view::getNumViewports(), 2);
  polyscope::show(3);

  // activateViewport / getActiveViewportIndex
  polyscope::view::activateViewport(1);
  EXPECT_EQ(polyscope::view::getActiveViewportIndex(), 1);
  polyscope::view::activateViewport(0);
  EXPECT_EQ(polyscope::view::getActiveViewportIndex(), 0);

  // shared navigation
  polyscope::view::setViewportsShareNavigation(true);
  EXPECT_TRUE(polyscope::view::getViewportsShareNavigation());
  polyscope::show(3);

  polyscope::view::setViewportsShareNavigation(false);
  EXPECT_FALSE(polyscope::view::getViewportsShareNavigation());
  polyscope::show(3);

  // 2x2 grid
  polyscope::view::createViewportGrid(2, 2);
  EXPECT_EQ(polyscope::view::getNumViewports(), 4);
  polyscope::show(3);

  // back to single viewport
  polyscope::view::createViewportGrid(1, 1);
  EXPECT_EQ(polyscope::view::getNumViewports(), 1);
  polyscope::show(3);

  polyscope::view::setViewportsShareNavigation(true); // restore default
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportVisibility) {
  auto psMesh = registerTriangleMesh();

  polyscope::view::createViewportGrid(2, 2); // 4 viewports

  // default: visible in all viewports
  for (int i = 0; i < 4; i++) EXPECT_TRUE(psMesh->getViewportVisibility(i));

  // hide in one viewport
  psMesh->setViewportVisibility(2, false);
  EXPECT_TRUE(psMesh->getViewportVisibility(0));
  EXPECT_TRUE(psMesh->getViewportVisibility(1));
  EXPECT_FALSE(psMesh->getViewportVisibility(2));
  EXPECT_TRUE(psMesh->getViewportVisibility(3));

  // hide all
  psMesh->setViewportVisibilityAll(false);
  for (int i = 0; i < 4; i++) EXPECT_FALSE(psMesh->getViewportVisibility(i));

  // show all
  psMesh->setViewportVisibilityAll(true);
  for (int i = 0; i < 4; i++) EXPECT_TRUE(psMesh->getViewportVisibility(i));

  // exclusive: only visible in viewport 1
  psMesh->setViewportVisibilityExclusive(1);
  EXPECT_FALSE(psMesh->getViewportVisibility(0));
  EXPECT_TRUE(psMesh->getViewportVisibility(1));
  EXPECT_FALSE(psMesh->getViewportVisibility(2));
  EXPECT_FALSE(psMesh->getViewportVisibility(3));

  // getViewportVisibility(-1) uses active viewport
  polyscope::view::activateViewport(1);
  EXPECT_TRUE(psMesh->getViewportVisibility(-1));
  polyscope::view::activateViewport(0);
  EXPECT_FALSE(psMesh->getViewportVisibility(-1));

  polyscope::show(3);

  polyscope::view::createViewportGrid(1, 1); // restore
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportProjectionMode) {
  auto psMesh = registerTriangleMesh();
  auto psCloud = polyscope::registerPointCloud("cloud", std::vector<glm::vec3>{{0, 0, 0}, {1, 0, 0}});

  polyscope::view::createViewportGrid(2, 1);           // 2 viewports
  polyscope::view::setViewportsShareNavigation(false); // allow independent projection modes

  // default: all viewports start with same projection mode (Perspective)
  polyscope::view::activateViewport(0);
  EXPECT_EQ(polyscope::view::getProjectionMode(), polyscope::ProjectionMode::Perspective);
  polyscope::view::activateViewport(1);
  EXPECT_EQ(polyscope::view::getProjectionMode(), polyscope::ProjectionMode::Perspective);

  // set viewport 1 to orthographic (stateful: set on the active viewport)
  polyscope::view::activateViewport(1);
  polyscope::view::setProjectionMode(polyscope::ProjectionMode::Orthographic);
  polyscope::view::activateViewport(0);
  EXPECT_EQ(polyscope::view::getProjectionMode(), polyscope::ProjectionMode::Perspective);
  polyscope::view::activateViewport(1);
  EXPECT_EQ(polyscope::view::getProjectionMode(), polyscope::ProjectionMode::Orthographic);

  // render with mixed projection modes
  polyscope::show(3);

  // restore
  polyscope::view::setViewportsShareNavigation(true);
  polyscope::view::setProjectionMode(polyscope::ProjectionMode::Perspective);
  polyscope::view::createViewportGrid(1, 1);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportCoordinates) {
  polyscope::view::setWindowSize(800, 600);
  polyscope::show(3); // ensure buffer is sized

  polyscope::view::createViewportGrid(2, 1); // left/right halves
  polyscope::show(3);

  int bufW = polyscope::view::bufferWidth;
  int bufH = polyscope::view::bufferHeight;

  // --- bufferIndsToScreenCoords / screenCoordsToBufferInds are inverses ---
  glm::ivec2 bi{bufW / 4, bufH / 4};
  glm::vec2 sc = polyscope::view::bufferIndsToScreenCoords(bi);
  auto [rx, ry] = polyscope::view::screenCoordsToBufferInds(sc);
  EXPECT_EQ(rx, bi.x);
  EXPECT_EQ(ry, bi.y);

  // --- viewport-local round-trip: buffer inds → screen coords → buffer inds ---
  glm::ivec2 localBi{10, 20};
  glm::vec2 localSc = polyscope::view::bufferIndsToScreenCoordsViewport(localBi, 0);
  auto [lx, ly] = polyscope::view::screenCoordsToBufferIndsViewport(localSc, 0);
  EXPECT_EQ(lx, localBi.x);
  EXPECT_EQ(ly, localBi.y);

  // --- globalToViewport / viewportToGlobal are inverses (screen coords) ---
  glm::vec2 globalSc = polyscope::view::bufferIndsToScreenCoords(bi);
  auto [localSc2, viewportInd] = polyscope::view::screenCoordsGlobalToViewport(globalSc);
  glm::vec2 recoveredSc = polyscope::view::screenCoordsViewportToGlobal(localSc2, viewportInd);
  EXPECT_NEAR(recoveredSc.x, globalSc.x, 1e-4f);
  EXPECT_NEAR(recoveredSc.y, globalSc.y, 1e-4f);

  // --- globalToViewport / viewportToGlobal are inverses (buffer inds) ---
  auto [localBi2, viewportInd2] = polyscope::view::bufferIndsGlobalToViewport(bi);
  glm::ivec2 recoveredBi = polyscope::view::bufferIndsViewportToGlobal(localBi2, viewportInd2);
  EXPECT_EQ(recoveredBi.x, bi.x);
  EXPECT_EQ(recoveredBi.y, bi.y);

  // --- screenCoordsToViewportInd and bufferIndsToViewportInd agree ---
  glm::vec2 testSc = polyscope::view::bufferIndsToScreenCoords(bi);
  int32_t indFromSc = polyscope::view::screenCoordsToViewportInd(testSc);
  int32_t indFromBi = polyscope::view::bufferIndsToViewportInd(bi);
  EXPECT_EQ(indFromSc, indFromBi);

  // --- point in left half should be viewport 0, right half viewport 1 ---
  glm::ivec2 leftBi{bufW / 4, bufH / 2};
  glm::ivec2 rightBi{3 * bufW / 4, bufH / 2};
  EXPECT_EQ(polyscope::view::bufferIndsToViewportInd(leftBi), 0);
  EXPECT_EQ(polyscope::view::bufferIndsToViewportInd(rightBi), 1);

  polyscope::view::createViewportGrid(1, 1); // restore
}

// A full-window viewport 0, with a small inset viewport 1 in the lower-right corner. Leaves viewport 0 active.
static void createInsetViewports() {
  polyscope::view::setNumViewports(2);
  polyscope::view::activateViewport(0);
  polyscope::view::setActiveViewportRelativeLocation(0.f, 0.f);
  polyscope::view::setActiveViewportRelativeSize(1.f, 1.f);
  polyscope::view::activateViewport(1);
  polyscope::view::setActiveViewportRelativeLocation(0.7f, 0.7f);
  polyscope::view::setActiveViewportRelativeSize(0.3f, 0.3f);
  polyscope::view::activateViewport(0);
}

// Unlike the 2x1 split above, an inset is not symmetric in y, so this catches y-from-top vs y-from-bottom mixups.
TEST_F(PolyscopeTest, ViewportCoordinatesInset) {
  polyscope::view::setWindowSize(800, 600);
  createInsetViewports();
  polyscope::show(3);

  float winW = polyscope::view::windowWidth;
  float winH = polyscope::view::windowHeight;
  glm::vec2 insetSc{0.85f * winW, 0.85f * winH};      // in the inset, at the lower-right
  glm::vec2 upperRightSc{0.85f * winW, 0.15f * winH}; // not in the inset

  // finding the viewport agrees for screen coords and buffer inds
  EXPECT_EQ(polyscope::view::screenCoordsToViewportInd(insetSc), 1);
  EXPECT_EQ(polyscope::view::screenCoordsToViewportInd(upperRightSc), 0);
  EXPECT_EQ(polyscope::view::bufferIndsToViewportInd(polyscope::view::screenCoordsToBufferIndsVec(insetSc)), 1);
  EXPECT_EQ(polyscope::view::bufferIndsToViewportInd(polyscope::view::screenCoordsToBufferIndsVec(upperRightSc)), 0);

  // global <--> local round trips. The center of the inset is 0.15 of the window from its top-left corner.
  auto [localSc, indSc] = polyscope::view::screenCoordsGlobalToViewport(insetSc);
  EXPECT_EQ(indSc, 1);
  EXPECT_NEAR(localSc.x, 0.15f * winW, 1e-3f);
  EXPECT_NEAR(localSc.y, 0.15f * winH, 1e-3f);
  EXPECT_NEAR(polyscope::view::screenCoordsViewportToGlobal(localSc, indSc).y, insetSc.y, 1e-3f);

  glm::ivec2 insetBi = polyscope::view::screenCoordsToBufferIndsVec(insetSc);
  auto [localBi, indBi] = polyscope::view::bufferIndsGlobalToViewport(insetBi);
  EXPECT_EQ(indBi, 1);
  EXPECT_EQ(polyscope::view::bufferIndsViewportToGlobal(localBi, indBi), insetBi);

  polyscope::view::createViewportGrid(1, 1); // restore
}

// The viewport ray helpers take local coordinates and use that viewport's camera (even if it isn't the active one), and
// the plain ones take global coordinates and find the viewport.
TEST_F(PolyscopeTest, ViewportRays) {
  polyscope::view::setWindowSize(800, 600);
  createInsetViewports();
  polyscope::view::setViewportsShareNavigation(false);
  polyscope::show(3);

  // cameras looking down different axes
  polyscope::view::activateViewport(1);
  polyscope::view::lookAt(glm::vec3{0.f, 0.f, 5.f}, glm::vec3{0.f, 0.f, 0.f}); // looks along -z
  polyscope::view::activateViewport(0);
  polyscope::view::lookAt(glm::vec3{5.f, 0.f, 0.f}, glm::vec3{0.f, 0.f, 0.f}); // looks along -x

  float winW = polyscope::view::windowWidth;
  float winH = polyscope::view::windowHeight;

  // the center of each viewport has a ray along that viewport's look direction (the active viewport is the default)
  EXPECT_NEAR(polyscope::view::screenCoordsToWorldRayViewport(glm::vec2{0.15f * winW, 0.15f * winH}, 1).z, -1.f, 1e-3f);
  EXPECT_NEAR(polyscope::view::screenCoordsToWorldRayViewport(glm::vec2{0.5f * winW, 0.5f * winH}).x, -1.f, 1e-3f);

  // the same with global coordinates
  EXPECT_NEAR(polyscope::view::screenCoordsToWorldRay(glm::vec2{0.85f * winW, 0.85f * winH}).z, -1.f, 1e-3f);
  EXPECT_NEAR(polyscope::view::screenCoordsToWorldRay(glm::vec2{0.5f * winW, 0.5f * winH}).x, -1.f, 1e-3f);

  polyscope::view::setViewportsShareNavigation(true);
  polyscope::view::createViewportGrid(1, 1); // restore
}

// With a single viewport, as in all code from before viewports, the plain ray function takes window coordinates and
// does not clamp them to the window.
TEST_F(PolyscopeTest, RayHelpersSingleViewport) {
  polyscope::view::setWindowSize(800, 600);
  polyscope::view::createViewportGrid(1, 1);
  polyscope::show(3);

  glm::vec2 outside{-300.f, 900.f};
  EXPECT_EQ(polyscope::view::screenCoordsToWorldRay(outside),
            polyscope::view::screenCoordsToWorldRayViewport(outside, 0));
  EXPECT_GT(glm::length(polyscope::view::screenCoordsToWorldRay(outside) -
                        polyscope::view::screenCoordsToWorldRay(glm::vec2{0.f, 599.f})),
            1e-2f);
}

// The mock backend can't return real hits, so this tests that picks are routed to the right viewport, with the right
// coordinates. (See the next test for real hits.)
TEST_F(PolyscopeTest, ViewportPickRouting) {
  polyscope::view::setWindowSize(800, 600);
  polyscope::registerPointCloud("cloud", std::vector<glm::vec3>{{0, 0, 0}, {1, 0, 0}});
  createInsetViewports();
  polyscope::show(3);

  float winW = polyscope::view::windowWidth;
  float winH = polyscope::view::windowHeight;
  const polyscope::view::ViewParameters& inset = polyscope::view::getViewportParameters(1);

  // a global point in the inset: the plain coordinates are global, the viewport ones are at the center of the inset
  glm::vec2 insetSc{0.85f * winW, 0.85f * winH};
  polyscope::PickResult pick = polyscope::pickAtScreenCoords(insetSc);
  EXPECT_EQ(pick.viewportInd, 1);
  EXPECT_NEAR(pick.screenCoords.x, insetSc.x, 1);
  EXPECT_NEAR(pick.screenCoords.y, insetSc.y, 1);
  EXPECT_NEAR(pick.viewportScreenCoords.x, 0.15f * winW, 1);
  EXPECT_NEAR(pick.viewportScreenCoords.y, 0.15f * winH, 1);
  EXPECT_NEAR(pick.viewportBufferInds.x, inset.viewportBufferWidth / 2, 1);
  EXPECT_NEAR(pick.viewportBufferInds.y, inset.viewportBufferHeight / 2, 1);

  // global buffer inds route the same way, and a pick can be reproduced from its local coordinates and viewport
  EXPECT_EQ(polyscope::pickAtBufferInds(pick.bufferInds).viewportInd, 1);
  EXPECT_EQ(polyscope::pickAtBufferIndsViewport(pick.viewportBufferInds, pick.viewportInd).bufferInds, pick.bufferInds);

  // a point outside the inset is in viewport 0, and picking does not change which viewport is active
  EXPECT_EQ(polyscope::pickAtScreenCoords(glm::vec2{0.3f * winW, 0.3f * winH}).viewportInd, 0);
  EXPECT_EQ(polyscope::view::getActiveViewportIndex(), 0);

  polyscope::view::createViewportGrid(1, 1); // restore
  polyscope::removeAllStructures();
}

// Pick which structure is really hit, with overlapping viewports. Needs real rendering, so it is skipped with the mock
// backend.
TEST_F(PolyscopeTest, ViewportPickHitsStructure) {
  if (testBackend == "openGL_mock") GTEST_SKIP() << "needs a real rendering backend to get real pick hits";

  polyscope::view::setWindowSize(800, 600);
  createInsetViewports();

  // Two big squares facing the camera: "A" at z=0 shown only in viewport 0, and "B" at z=1 shown only in the inset
  auto makeSquare = [](float z) {
    return std::vector<glm::vec3>{{-10, -10, z}, {10, -10, z}, {10, 10, z}, {-10, 10, z}};
  };
  std::vector<std::vector<size_t>> faces{{0, 1, 2}, {0, 2, 3}};
  polyscope::registerSurfaceMesh("A", makeSquare(0.f), faces)->setViewportVisibilityExclusive(0);
  polyscope::registerSurfaceMesh("B", makeSquare(1.f), faces)->setViewportVisibilityExclusive(1);
  polyscope::view::lookAt(glm::vec3{0.f, 0.f, 5.f}, glm::vec3{0.f, 0.f, 0.f});
  polyscope::show(3);

  float winW = polyscope::view::windowWidth;
  float winH = polyscope::view::windowHeight;
  glm::vec2 insetSc{0.85f * winW, 0.85f * winH};

  // a global point in the inset hits "B", at its depth
  polyscope::PickResult insetPick = polyscope::pickAtScreenCoords(insetSc);
  EXPECT_EQ(insetPick.structureName, "B");
  EXPECT_EQ(insetPick.viewportInd, 1);
  EXPECT_NEAR(insetPick.position.z, 1.f, 0.05f);

  // a global point outside the inset hits "A"
  EXPECT_EQ(polyscope::pickAtScreenCoords(glm::vec2{0.3f * winW, 0.3f * winH}).structureName, "A");

  // asking for viewport 0 at the same point as the inset hits "A", the structure underneath the inset
  EXPECT_EQ(polyscope::pickAtScreenCoordsViewport(insetSc, 0).structureName, "A");

  // top-row pick (y=0) must not read out-of-bounds
  polyscope::PickResult topRowPick = polyscope::pickAtBufferIndsViewport(glm::ivec2{10, 0}, 0);
  EXPECT_EQ(topRowPick.structureName, "A");

  polyscope::view::createViewportGrid(1, 1); // restore
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, UIScale) {

  // this should trigger a reload of the font atlas

  polyscope::options::uiScale = 1.25;
  polyscope::show(3);

  polyscope::options::uiScale = 1.;
  polyscope::show(3);
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

  // render with viewports
  polyscope::view::createViewportGrid(2, 1);
  polyscope::show(3);
  polyscope::view::createViewportGrid(1, 1);

  polyscope::removeAllStructures();
}
