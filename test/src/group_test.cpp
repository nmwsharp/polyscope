// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"

// ============================================================
// =============== Group tests
// ============================================================

TEST_F(PolyscopeTest, RegisterGroupTest) {
  polyscope::registerGroup("test_group");
  polyscope::show(3);

  polyscope::removeAllGroups();
}

TEST_F(PolyscopeTest, AddStructuresToGroupTest) {
  std::string cloudName = "test_point_cloud";
  std::string curveName = "test_curve_network";
  std::string meshName = "test_triangle_mesh";
  // Add a point cloud structure
  auto psCloud = registerPointCloud(cloudName);
  // Add a curve network structure
  auto psCurve = registerCurveNetwork(curveName);
  // Add a triangle mesh structure
  auto psMesh = registerTriangleMesh(meshName);
  polyscope::registerGroup("test_group");
  polyscope::setParentGroupOfStructure(psCloud, "test_group");
  polyscope::setParentGroupOfStructure(psCurve, "test_group");
  polyscope::setParentGroupOfStructure(psMesh, "test_group");
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, AddGroupsToGroupTest) {
  polyscope::registerGroup("test_group");
  polyscope::registerGroup("child_test_group_1");
  polyscope::registerGroup("child_test_group_2");
  polyscope::setParentGroupOfGroup("child_test_group_1", "test_group");
  polyscope::setParentGroupOfGroup("child_test_group_2", "test_group");
  polyscope::show(3);

  polyscope::removeAllGroups();
}

TEST_F(PolyscopeTest, AddStructuresAndGroupsToGroupTest) {
  std::string cloudName = "test_point_cloud";
  std::string curveName = "test_curve_network";
  std::string meshName = "test_triangle_mesh";
  auto psCloud1 = registerPointCloud(cloudName + "1");
  auto psCloud2 = registerPointCloud(cloudName + "2");
  auto psCloud3 = registerPointCloud(cloudName + "3");
  auto psCurve1 = registerCurveNetwork(curveName + "1");
  auto psCurve2 = registerCurveNetwork(curveName + "2");
  auto psCurve3 = registerCurveNetwork(curveName + "3");
  auto psMesh1 = registerTriangleMesh(meshName + "1");
  auto psMesh2 = registerTriangleMesh(meshName + "2");
  auto psMesh3 = registerTriangleMesh(meshName + "3");
  polyscope::registerGroup("test_group");
  polyscope::registerGroup("points_group");
  polyscope::registerGroup("curves_group");
  polyscope::registerGroup("meshes_group");
  polyscope::setParentGroupOfStructure(psCloud1, "points_group");
  polyscope::setParentGroupOfStructure(psCloud2, "points_group");
  polyscope::setParentGroupOfStructure(psCloud3, "points_group");
  polyscope::setParentGroupOfStructure(psCurve1, "curves_group");
  polyscope::setParentGroupOfStructure(psCurve2, "curves_group");
  polyscope::setParentGroupOfStructure(psCurve3, "curves_group");
  polyscope::setParentGroupOfStructure(psMesh1, "meshes_group");
  polyscope::setParentGroupOfStructure(psMesh2, "meshes_group");
  polyscope::setParentGroupOfStructure(psMesh3, "meshes_group");
  polyscope::setParentGroupOfGroup("points_group", "test_group");
  polyscope::setParentGroupOfGroup("curves_group", "test_group");
  polyscope::setParentGroupOfGroup("meshes_group", "test_group");
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, RemoveStructureButLeaveItInGroupTest) {
  std::string cloudName = "test_point_cloud";
  std::string curveName = "test_curve_network";
  std::string meshName = "test_triangle_mesh";
  // Add a point cloud structure
  auto psCloud = registerPointCloud(cloudName);
  // Add a curve network structure
  auto psCurve = registerCurveNetwork(curveName);
  // Add a triangle mesh structure
  auto psMesh = registerTriangleMesh(meshName);
  polyscope::registerGroup("test_group");
  polyscope::setParentGroupOfStructure(psCloud, "test_group");
  polyscope::setParentGroupOfStructure(psCurve, "test_group");
  polyscope::setParentGroupOfStructure(psMesh, "test_group");
  polyscope::removeStructure(cloudName);
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestDisableGroup) {
  std::string cloudName = "test_point_cloud";
  std::string curveName = "test_curve_network";
  std::string meshName = "test_triangle_mesh";
  // Add a point cloud structure
  auto psCloud = registerPointCloud(cloudName);
  // Add a curve network structure
  auto psCurve = registerCurveNetwork(curveName);
  // Add a triangle mesh structure
  auto psMesh = registerTriangleMesh(meshName);
  polyscope::registerGroup("test_group");
  polyscope::setParentGroupOfStructure(psCloud, "test_group");
  polyscope::setParentGroupOfStructure(psCurve, "test_group");
  polyscope::setParentGroupOfStructure(psMesh, "test_group");
  polyscope::setGroupEnabled("test_group", false);
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestDisableSubgroup) {
  std::string cloudName = "test_point_cloud";
  std::string curveName = "test_curve_network";
  std::string meshName = "test_triangle_mesh";
  auto psCloud1 = registerPointCloud(cloudName + "1");
  auto psCloud2 = registerPointCloud(cloudName + "2");
  auto psCloud3 = registerPointCloud(cloudName + "3");
  auto psCurve1 = registerCurveNetwork(curveName + "1");
  auto psCurve2 = registerCurveNetwork(curveName + "2");
  auto psCurve3 = registerCurveNetwork(curveName + "3");
  auto psMesh1 = registerTriangleMesh(meshName + "1");
  auto psMesh2 = registerTriangleMesh(meshName + "2");
  auto psMesh3 = registerTriangleMesh(meshName + "3");
  polyscope::registerGroup("test_group");
  polyscope::registerGroup("points_group");
  polyscope::registerGroup("curves_group");
  polyscope::registerGroup("meshes_group");
  polyscope::setParentGroupOfStructure(psCloud1, "points_group");
  polyscope::setParentGroupOfStructure(psCloud2, "points_group");
  polyscope::setParentGroupOfStructure(psCloud3, "points_group");
  polyscope::setParentGroupOfStructure(psCurve1, "curves_group");
  polyscope::setParentGroupOfStructure(psCurve2, "curves_group");
  polyscope::setParentGroupOfStructure(psCurve3, "curves_group");
  polyscope::setParentGroupOfStructure(psMesh1, "meshes_group");
  polyscope::setParentGroupOfStructure(psMesh2, "meshes_group");
  polyscope::setParentGroupOfStructure(psMesh3, "meshes_group");
  polyscope::setParentGroupOfGroup("points_group", "test_group");
  polyscope::setParentGroupOfGroup("curves_group", "test_group");
  polyscope::setParentGroupOfGroup("meshes_group", "test_group");
  polyscope::setGroupEnabled("meshes_group", false);
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestRemoveSubgroup) {
  // both remaining groups should remain and be root groups
  auto psCurve1 = registerCurveNetwork("test_curve");
  polyscope::registerGroup("test_group");
  polyscope::registerGroup("test_child_group");
  polyscope::registerGroup("test_grandchild_group");
  polyscope::setParentGroupOfGroup("test_child_group", "test_group");
  polyscope::setParentGroupOfGroup("test_grandchild_group", "test_child_group");
  polyscope::setParentGroupOfStructure(psCurve1, "test_grandchild_group");
  polyscope::removeGroup("test_child_group");
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestRepeatAddAndRemoveGroup) {
  auto psCurve1 = registerCurveNetwork("test_curve");
  polyscope::registerGroup("test_group");
  for (int i = 0; i < 10; i++) {
    polyscope::registerGroup("test_child_group");
    polyscope::setParentGroupOfGroup("test_child_group", "test_group");
    polyscope::setParentGroupOfStructure(psCurve1, "test_child_group");
    if (i != 9) {
      polyscope::removeGroup("test_child_group");
    }
  }
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestDocsExample) {
  // make a point cloud
  std::vector<glm::vec3> points;
  for (size_t i = 0; i < 3000; i++) {
    points.push_back(
        glm::vec3{polyscope::randomUnit() - .5, polyscope::randomUnit() - .5, polyscope::randomUnit() - .5});
  }
  polyscope::PointCloud* psCloud = polyscope::registerPointCloud("my cloud", points);
  psCloud->setPointRadius(0.02);
  psCloud->setPointRenderMode(polyscope::PointRenderMode::Quad);

  // make a curve network
  std::vector<glm::vec3> nodes;
  std::vector<std::array<size_t, 2>> edges;
  nodes = {
      {1, 0, 0},
      {0, 1, 0},
      {0, 0, 1},
      {0, 0, 0},
  };
  edges = {{1, 3}, {3, 0}, {1, 0}, {0, 2}};
  polyscope::CurveNetwork* psCurve = polyscope::registerCurveNetwork("my network", nodes, edges);

  // group them together
  std::string groupName = "my group";
  polyscope::registerGroup(groupName);
  polyscope::setParentGroupOfStructure(psCloud, groupName);
  polyscope::setParentGroupOfStructure(psCurve, groupName);

  // put that group in another group
  std::string parentGroupName = "my parent group";
  std::string emptyGroupName = "my empty group";
  polyscope::registerGroup(parentGroupName);
  polyscope::registerGroup(emptyGroupName);
  polyscope::setParentGroupOfGroup(groupName, parentGroupName);
  polyscope::setParentGroupOfGroup(emptyGroupName, parentGroupName);

  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestDeletedGroupReferenceError) {
  // don't run this because we can't catch UI errors
  if (true) return;
  polyscope::registerGroup("test_group");
  polyscope::registerGroup("test_child_group");
  polyscope::removeGroup("test_group");
  // this should throw an error (but not segfault)
  polyscope::setParentGroupOfGroup("test_child_group", "test_group");
}

TEST_F(PolyscopeTest, TestGroupCycleError) {
  // don't run this because we can't catch UI errors
  if (true) return;
  polyscope::registerGroup("test_group");
  polyscope::registerGroup("test_child_group");
  polyscope::setParentGroupOfGroup("test_child_group", "test_group");
  // this should throw an error (but not segfault)
  polyscope::setParentGroupOfGroup("test_group", "test_child_group");
}
