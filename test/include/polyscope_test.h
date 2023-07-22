// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "polyscope/camera_view.h"
#include "polyscope/curve_network.h"
#include "polyscope/implicit_helpers.h"
#include "polyscope/pick.h"
#include "polyscope/point_cloud.h"
#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/types.h"
#include "polyscope/volume_mesh.h"

// Which polyscope backend to use for testing
extern std::string testBackend;


class PolyscopeTest : public ::testing::Test {
protected:
  // Per-test-suite set-up.
  // Called before the first test in this test suite.
  // Can be omitted if not needed.
  static void SetUpTestSuite() {
    polyscope::init(testBackend);
    polyscope::options::enableRenderErrorChecks = true;
  }

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

// == Common helpers

inline std::vector<glm::vec3> getPoints() {
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



inline polyscope::PointCloud* registerPointCloud(std::string name = "test1") {
  std::vector<glm::vec3> points = getPoints();
  polyscope::PointCloud* psPoints = polyscope::registerPointCloud(name, points);
  psPoints->setPointRenderMode(polyscope::PointRenderMode::Sphere);
  return psPoints;
}

inline std::tuple<std::vector<glm::vec3>, std::vector<std::vector<size_t>>> getTriangleMesh() {
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

inline polyscope::SurfaceMesh* registerTriangleMesh(std::string name = "test1") {
  std::vector<glm::vec3> points;
  std::vector<std::vector<size_t>> faces;
  std::tie(points, faces) = getTriangleMesh();
  return polyscope::registerSurfaceMesh(name, points, faces);
}

inline std::tuple<std::vector<glm::vec3>, std::vector<std::array<size_t, 2>>> getCurveNetwork() {
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

inline polyscope::CurveNetwork* registerCurveNetwork(std::string name = "test1") {
  std::vector<glm::vec3> points;
  std::vector<std::array<size_t, 2>> edges;
  std::tie(points, edges) = getCurveNetwork();
  return polyscope::registerCurveNetwork(name, points, edges);
}

inline std::tuple<std::vector<glm::vec3>, std::vector<std::array<int, 8>>> getVolumeMeshData() {
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
