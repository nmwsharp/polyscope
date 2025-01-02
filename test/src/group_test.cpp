// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"

// ============================================================
// =============== Group tests
// ============================================================

TEST_F(PolyscopeTest, RegisterGroupTest) {
  polyscope::createGroup("test_group");
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
  polyscope::createGroup("test_group");
  psCloud->addToGroup("test_group");
  psMesh->addToGroup("test_group");
  psCurve->addToGroup("test_group");
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, AddGroupsToGroupTest) {
  polyscope::Group* gTest = polyscope::createGroup("test_group");
  polyscope::Group* gTest1 = polyscope::createGroup("child_test_group_1");
  polyscope::Group* gTest2 = polyscope::createGroup("child_test_group_2");
  gTest->addChildGroup(*gTest1);
  gTest->addChildGroup(*gTest2);
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
  polyscope::Group* test_group = polyscope::createGroup("test_group");
  polyscope::Group* points_group = polyscope::createGroup("points_group");
  polyscope::Group* curves_group = polyscope::createGroup("curves_group");
  polyscope::Group* meshes_group = polyscope::createGroup("meshes_group");
  psCloud1->addToGroup("points_group");
  psCloud2->addToGroup(*points_group);
  points_group->addChildStructure(*psCloud3);
  psCurve1->addToGroup("curves_group");
  psCurve2->addToGroup("curves_group");
  psCurve3->addToGroup("curves_group");
  psMesh1->addToGroup("meshes_group");
  psMesh2->addToGroup(*meshes_group);
  psMesh3->addToGroup("meshes_group");
  test_group->addChildGroup(*points_group);
  test_group->addChildGroup(*curves_group);
  test_group->addChildGroup(*meshes_group);
  polyscope::show(3);

  // test a few options


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
  polyscope::createGroup("test_group");
  psCloud->addToGroup("test_group");
  psCurve->addToGroup("test_group");
  psMesh->addToGroup("test_group");
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
  polyscope::Group* test_group = polyscope::createGroup("test_group");
  psCloud->addToGroup("test_group");
  psCurve->addToGroup("test_group");
  psMesh->addToGroup("test_group");
  test_group->setEnabled(false);
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
  polyscope::Group* test_group = polyscope::createGroup("test_group");
  polyscope::Group* points_group = polyscope::createGroup("points_group");
  polyscope::Group* curves_group = polyscope::createGroup("curves_group");
  polyscope::Group* meshes_group = polyscope::createGroup("meshes_group");
  psCloud1->addToGroup("points_group");
  psCloud2->addToGroup("points_group");
  psCloud3->addToGroup("points_group");
  psCurve1->addToGroup("curves_group");
  psCurve2->addToGroup("curves_group");
  psCurve3->addToGroup("curves_group");
  psMesh1->addToGroup("meshes_group");
  psMesh2->addToGroup("meshes_group");
  psMesh3->addToGroup("meshes_group");
  test_group->addChildGroup(*points_group);
  test_group->addChildGroup(*curves_group);
  test_group->addChildGroup(*meshes_group);
  test_group->setEnabled(false);
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestRemoveSubgroup) {
  // both remaining groups should remain and be root groups
  auto psCurve1 = registerCurveNetwork("test_curve");
  polyscope::Group* test_group = polyscope::createGroup("test_group");
  polyscope::Group* test_child_group = polyscope::createGroup("test_child_group");
  polyscope::Group* test_grandchild_group = polyscope::createGroup("test_grandchild_group");
  test_group->addChildGroup(*test_child_group);
  test_child_group->addChildGroup(*test_grandchild_group);
  psCurve1->addToGroup("test_grandchild_group");
  polyscope::removeGroup("test_child_group");
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestRepeatAddAndRemoveGroup) {
  auto psCurve1 = registerCurveNetwork("test_curve");
  polyscope::Group* test_group = polyscope::createGroup("test_group");
  for (int i = 0; i < 10; i++) {
    polyscope::Group* test_child_group = polyscope::createGroup("test_child_group");
    test_group->addChildGroup(*test_child_group);
    psCurve1->addToGroup("test_child_group");
    if (i != 9) {
      polyscope::removeGroup(test_child_group);
    }
  }
  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestDocsExample) {

  // make a point cloud
  std::vector<glm::vec3> points;
  for (size_t i = 0; i < 300; i++) {
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

  // create a group for these two objects
  std::string groupName = "my group";
  polyscope::Group* group = polyscope::createGroup(groupName);
  psCurve->addToGroup(*group);    // add by group ref
  psCloud->addToGroup(groupName); // add by name

  // toggle enabled for everything in the group
  group->setEnabled(false);

  // hide items in group from displaying in the UI
  // (useful if you are registering huge numbers of structures you don't always need to see)
  group->setHideDescendantsFromStructureLists(true);
  group->setShowChildDetails(false);

  // nest groups inside of other groups
  std::string superGroupName = "my parent group";
  polyscope::Group* superGroup = polyscope::createGroup(superGroupName);
  superGroup->addChildGroup(*group);

  polyscope::show(3);

  polyscope::removeAllGroups();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, TestDeletedGroupReferenceError) {
  // don't run this because we can't catch UI errors
  if (true) return;
  polyscope::Group* test_group = polyscope::createGroup("test_group");
  polyscope::Group* test_child_group = polyscope::createGroup("test_child_group");
  polyscope::removeGroup("test_group");
  // this should throw an error (but not segfault)
  polyscope::getGroup("test_group");
}

TEST_F(PolyscopeTest, TestGroupCycleError) {
  // don't run this because we can't catch UI errors
  if (true) return;
  polyscope::Group* test_group = polyscope::createGroup("test_group");
  polyscope::Group* test_child_group = polyscope::createGroup("test_child_group");
  test_group->addChildGroup(*test_child_group);
  // this should throw an error (but not segfault)
  test_child_group->addChildGroup(*test_group);
}
