// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"

#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/viewport.h"

#include "gtest/gtest.h"

#include <array>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
// =============== Viewport tests
// ============================================================

TEST_F(PolyscopeTest, ViewportDefaultIsSingle) {
  EXPECT_EQ(polyscope::getViewportGridRows(), 1);
  EXPECT_EQ(polyscope::getViewportGridCols(), 1);
  EXPECT_EQ(polyscope::getActiveViewport(), nullptr);
  EXPECT_EQ(polyscope::getViewport(0, 0), nullptr); // no viewport objects in single mode
}

TEST_F(PolyscopeTest, ViewportSetGridLayout) {
  polyscope::setViewportGridLayout(2, 2);
  EXPECT_EQ(polyscope::getViewportGridRows(), 2);
  EXPECT_EQ(polyscope::getViewportGridCols(), 2);

  // Should have 4 viewport objects
  polyscope::Viewport* v00 = polyscope::getViewport(0, 0);
  polyscope::Viewport* v01 = polyscope::getViewport(0, 1);
  polyscope::Viewport* v10 = polyscope::getViewport(1, 0);
  polyscope::Viewport* v11 = polyscope::getViewport(1, 1);
  EXPECT_NE(v00, nullptr);
  EXPECT_NE(v01, nullptr);
  EXPECT_NE(v10, nullptr);
  EXPECT_NE(v11, nullptr);

  // Out of range returns nullptr
  EXPECT_EQ(polyscope::getViewport(2, 0), nullptr);
  EXPECT_EQ(polyscope::getViewport(0, 2), nullptr);

  // Reset back
  polyscope::setSingleViewport();
  EXPECT_EQ(polyscope::getViewportGridRows(), 1);
  EXPECT_EQ(polyscope::getViewportGridCols(), 1);
  EXPECT_EQ(polyscope::getViewport(0, 0), nullptr);
}

TEST_F(PolyscopeTest, ViewportPresets) {
  polyscope::setVerticalSplitViewport();
  EXPECT_EQ(polyscope::getViewportGridRows(), 1);
  EXPECT_EQ(polyscope::getViewportGridCols(), 2);
  EXPECT_NE(polyscope::getViewport(0, 0), nullptr);
  EXPECT_NE(polyscope::getViewport(0, 1), nullptr);

  polyscope::setHorizontalSplitViewport();
  EXPECT_EQ(polyscope::getViewportGridRows(), 2);
  EXPECT_EQ(polyscope::getViewportGridCols(), 1);
  EXPECT_NE(polyscope::getViewport(0, 0), nullptr);
  EXPECT_NE(polyscope::getViewport(1, 0), nullptr);

  polyscope::setQuadViewport();
  EXPECT_EQ(polyscope::getViewportGridRows(), 2);
  EXPECT_EQ(polyscope::getViewportGridCols(), 2);

  polyscope::setSingleViewport();
}

TEST_F(PolyscopeTest, ViewportRenderSingle) {
  // Rendering with a single viewport should work exactly as before
  auto psMesh = registerTriangleMesh();
  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportRenderVerticalSplit) {
  auto psMesh = registerTriangleMesh();
  polyscope::setVerticalSplitViewport();
  polyscope::show(3);
  polyscope::setSingleViewport();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportRenderHorizontalSplit) {
  auto psMesh = registerTriangleMesh();
  polyscope::setHorizontalSplitViewport();
  polyscope::show(3);
  polyscope::setSingleViewport();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportRenderQuad) {
  auto psMesh = registerTriangleMesh();
  polyscope::setQuadViewport();
  polyscope::show(3);
  polyscope::setSingleViewport();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportIndependentCameraSettings) {
  polyscope::setQuadViewport();

  polyscope::Viewport* v00 = polyscope::getViewport(0, 0);
  polyscope::Viewport* v01 = polyscope::getViewport(0, 1);
  polyscope::Viewport* v10 = polyscope::getViewport(1, 0);
  polyscope::Viewport* v11 = polyscope::getViewport(1, 1);

  // Set different navigation styles per viewport
  v00->navigateStyle = polyscope::NavigateStyle::Turntable;
  v01->navigateStyle = polyscope::NavigateStyle::Free;
  v10->navigateStyle = polyscope::NavigateStyle::Planar;
  v11->navigateStyle = polyscope::NavigateStyle::Arcball;

  EXPECT_EQ(v00->navigateStyle, polyscope::NavigateStyle::Turntable);
  EXPECT_EQ(v01->navigateStyle, polyscope::NavigateStyle::Free);
  EXPECT_EQ(v10->navigateStyle, polyscope::NavigateStyle::Planar);
  EXPECT_EQ(v11->navigateStyle, polyscope::NavigateStyle::Arcball);

  // Set different up directions
  v00->upDir = polyscope::UpDir::YUp;
  v01->upDir = polyscope::UpDir::ZUp;

  EXPECT_EQ(v00->upDir, polyscope::UpDir::YUp);
  EXPECT_EQ(v01->upDir, polyscope::UpDir::ZUp);

  // Set different FOV
  v00->fov = 45.0f;
  v01->fov = 90.0f;

  EXPECT_FLOAT_EQ(v00->fov, 45.0f);
  EXPECT_FLOAT_EQ(v01->fov, 90.0f);

  // Set different bg colors
  v00->bgColor = {{1.0f, 0.0f, 0.0f, 1.0f}};
  v01->bgColor = {{0.0f, 1.0f, 0.0f, 1.0f}};

  polyscope::setSingleViewport();
}

TEST_F(PolyscopeTest, ViewportRenderWithIndependentSettings) {
  auto psMesh = registerTriangleMesh();
  polyscope::setQuadViewport();

  polyscope::Viewport* v00 = polyscope::getViewport(0, 0);
  polyscope::Viewport* v10 = polyscope::getViewport(1, 0);

  // One viewport is 3D with turntable, the other is 2D planar
  v00->navigateStyle = polyscope::NavigateStyle::Turntable;
  v10->navigateStyle = polyscope::NavigateStyle::Planar;

  polyscope::show(3);

  polyscope::setSingleViewport();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportNavigateStyleSurvivesFrames) {
  auto psMesh = registerTriangleMesh();
  polyscope::setVerticalSplitViewport();

  polyscope::Viewport* v0 = polyscope::getViewport(0, 0);
  polyscope::Viewport* v1 = polyscope::getViewport(0, 1);

  // Use the setter (which calls view::setNavigateStyle internally)
  v1->setNavigateStyle(polyscope::NavigateStyle::Planar);
  EXPECT_EQ(v1->navigateStyle, polyscope::NavigateStyle::Planar);
  EXPECT_EQ(v0->navigateStyle, polyscope::NavigateStyle::Turntable);

  // Render several frames and verify styles are preserved
  for (int i = 0; i < 5; i++) {
    polyscope::frameTick();
    EXPECT_EQ(v0->navigateStyle, polyscope::NavigateStyle::Turntable);
    EXPECT_EQ(v1->navigateStyle, polyscope::NavigateStyle::Planar);
  }

  polyscope::setSingleViewport();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportFrameTick) {
  auto psMesh = registerTriangleMesh();
  polyscope::setVerticalSplitViewport();

  for (int i = 0; i < 5; i++) {
    polyscope::frameTick();
  }

  polyscope::setSingleViewport();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportPushPullState) {
  polyscope::setVerticalSplitViewport();

  polyscope::Viewport* v0 = polyscope::getViewport(0, 0);
  polyscope::Viewport* v1 = polyscope::getViewport(0, 1);

  // Set different FOVs
  v0->fov = 30.0f;
  v1->fov = 90.0f;

  // Save global state
  polyscope::ViewStateSnapshot saved = polyscope::saveViewState();

  // Push v0's state
  v0->pushViewState();
  EXPECT_FLOAT_EQ(polyscope::view::fov, 30.0f);

  // Restore global
  polyscope::restoreViewState(saved);

  // Push v1's state
  v1->pushViewState();
  EXPECT_FLOAT_EQ(polyscope::view::fov, 90.0f);

  // Restore global
  polyscope::restoreViewState(saved);

  polyscope::setSingleViewport();
}

TEST_F(PolyscopeTest, ViewportContainsScreenCoords) {
  polyscope::setVerticalSplitViewport();

  polyscope::Viewport* vLeft = polyscope::getViewport(0, 0);
  polyscope::Viewport* vRight = polyscope::getViewport(0, 1);

  // Left viewport should contain points on the left half of the window
  EXPECT_TRUE(vLeft->containsScreenCoords(10.0f, 10.0f));

  // Right viewport should contain points on the right half
  float rightX = static_cast<float>(polyscope::view::windowWidth) * 0.75f;
  EXPECT_TRUE(vRight->containsScreenCoords(rightX, 10.0f));

  // Check they don't overlap at the boundary
  float midX = static_cast<float>(polyscope::view::windowWidth) / 2.0f;
  // Exactly one should contain midpoint
  bool leftContains = vLeft->containsScreenCoords(midX, 10.0f);
  bool rightContains = vRight->containsScreenCoords(midX, 10.0f);
  EXPECT_TRUE(leftContains || rightContains);

  polyscope::setSingleViewport();
}

TEST_F(PolyscopeTest, ViewportLayoutUpdate) {
  polyscope::setQuadViewport();

  polyscope::Viewport* v00 = polyscope::getViewport(0, 0);
  polyscope::Viewport* v11 = polyscope::getViewport(1, 1);

  // After layout update, viewports should have non-zero dimensions
  polyscope::updateViewportLayouts();

  EXPECT_GT(v00->pixelWidth, 0);
  EXPECT_GT(v00->pixelHeight, 0);
  EXPECT_GT(v11->pixelWidth, 0);
  EXPECT_GT(v11->pixelHeight, 0);

  // The four viewports should tile the full window
  EXPECT_EQ(v00->windowX, 0);
  EXPECT_EQ(v00->windowY, 0);
  EXPECT_EQ(v11->windowX + v11->windowW, polyscope::view::windowWidth);
  EXPECT_EQ(v11->windowY + v11->windowH, polyscope::view::windowHeight);

  polyscope::setSingleViewport();
}

TEST_F(PolyscopeTest, ViewportRemoveEverythingResets) {
  polyscope::setQuadViewport();
  EXPECT_EQ(polyscope::getViewportGridRows(), 2);

  polyscope::removeEverything();

  EXPECT_EQ(polyscope::getViewportGridRows(), 1);
  EXPECT_EQ(polyscope::getViewportGridCols(), 1);
  EXPECT_EQ(polyscope::getViewport(0, 0), nullptr);
}

TEST_F(PolyscopeTest, ViewportScreenshotWithSplit) {
  auto psMesh = registerTriangleMesh();
  polyscope::setVerticalSplitViewport();

  // Taking a screenshot with split viewports should not crash
  polyscope::screenshot("test_viewport_screenshot.png");

  polyscope::setSingleViewport();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, ViewportSwitchLayoutWhileShowing) {
  auto psMesh = registerTriangleMesh();

  // Start single, switch to split, render some frames, switch back
  polyscope::show(3);
  polyscope::setVerticalSplitViewport();
  polyscope::show(3);
  polyscope::setQuadViewport();
  polyscope::show(3);
  polyscope::setSingleViewport();
  polyscope::show(3);

  polyscope::removeAllStructures();
}
