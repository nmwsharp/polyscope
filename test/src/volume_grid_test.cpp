// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/slice_plane.h"
#include "polyscope_test.h"


// ============================================================
// =============== Volume grid tests
// ============================================================

TEST_F(PolyscopeTest, ShowVolumeGrid) {
  // clang-format off
  uint32_t dimX = 8;
  uint32_t dimY = 10;
  uint32_t dimZ = 12;
  glm::vec3 bound_low{-3., -3., -3.};
  glm::vec3 bound_high{3., 3., 3.};

  polyscope::VolumeGrid* psGrid = polyscope::registerVolumeGrid("test grid", {dimX, dimY, dimZ}, bound_low, bound_high);

  polyscope::show(3);
  
  EXPECT_TRUE(polyscope::hasVolumeGrid("test grid"));
  EXPECT_FALSE(polyscope::hasVolumeGrid("other grid"));
  polyscope::removeAllStructures();
  EXPECT_FALSE(polyscope::hasVolumeGrid("test grid"));
}

TEST_F(PolyscopeTest, VolumeGridBasicOptions) {
  
  // these are node dim
  uint32_t dimX = 8;
  uint32_t dimY = 10;
  uint32_t dimZ = 12;
  glm::vec3 bound_low{-3., -3., -3.};
  glm::vec3 bound_high{3., 3., 3.};

  polyscope::VolumeGrid* psGrid = polyscope::registerVolumeGrid("test grid", {dimX, dimY, dimZ}, bound_low, bound_high);

  EXPECT_EQ(psGrid->nNodes(), dimX*dimY*dimZ);
  EXPECT_EQ(psGrid->nCells(), (dimX-1)*(dimY-1)*(dimZ-1));

  // Material
  psGrid->setMaterial("flat");
  EXPECT_EQ(psGrid->getMaterial(), "flat");
  polyscope::show(3);

  // Edge width
  psGrid->setEdgeWidth(0.5);
  polyscope::show(3);
  
  // Grid size factor
  psGrid->setCubeSizeFactor(0.5);
  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, VolumeGridSlicePlane) {
  
  // these are node dim
  uint32_t dimX = 8;
  uint32_t dimY = 10;
  uint32_t dimZ = 12;
  glm::vec3 bound_low{-3., -3., -3.};
  glm::vec3 bound_high{3., 3., 3.};

  polyscope::VolumeGrid* psGrid = polyscope::registerVolumeGrid("test grid", {dimX, dimY, dimZ}, bound_low, bound_high);

  // plain old inspecting
  polyscope::SlicePlane* p = polyscope::addSceneSlicePlane();
  psGrid->setCullWholeElements(true); 
  polyscope::show(3);
  
  // cull whole elements
  // we don't actually support rendering like this yet, so right now this is 'handled' by automatically unsetting it internally
  psGrid->setCullWholeElements(false); 
  polyscope::show(3);

  polyscope::removeLastSceneSlicePlane();
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, VolumeGridScalar) {
  
  // these are node dim
  uint32_t dimX = 8;
  uint32_t dimY = 10;
  uint32_t dimZ = 12;
  glm::vec3 bound_low{-3., -3., -3.};
  glm::vec3 bound_high{3., 3., 3.};

  polyscope::VolumeGrid* psGrid = polyscope::registerVolumeGrid("test grid", {dimX, dimY, dimZ}, bound_low, bound_high);

  // callable, for callable variants below
  auto torusSDF = [](glm::vec3 p) {
    float scale = 0.5;
    p /= scale;
    p += glm::vec3{1., 0., 1.};
    glm::vec2 t{1., 0.3};
    glm::vec2 pxz{p.x, p.z};
    glm::vec2 q = glm::vec2(glm::length(pxz) - t.x, p.y);
    return (glm::length(q) - t.y) * scale;
  };

  { // node scalar from array
    std::vector<double> nodeScalar(psGrid->nNodes(), 3.0f);
    psGrid->addNodeScalarQuantity("node scalar1", nodeScalar)->setEnabled(true);
    polyscope::show(3);
  }
  
  { // node scalar from callable
    // internally this bootstraps off the batch version, so we're kinda testing it too
    psGrid->addNodeScalarQuantityFromCallable("node scalar2", torusSDF)->setEnabled(true);
    polyscope::show(3);
  }
  
  { // cell scalar from array
    std::vector<double> cellScalar(psGrid->nCells(), 3.0f);
    psGrid->addCellScalarQuantity("cell scalar1", cellScalar)->setEnabled(true);
    polyscope::show(3);
  }
  
  { // cell scalar from callable
    // internally this bootstraps off the batch version, so we're kinda testing it too
    psGrid->addCellScalarQuantityFromCallable("cell scalar2", torusSDF)->setEnabled(true);
    polyscope::show(3);
  }

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, VolumeGridScalarIsosurfaceAndOpts) {
  
  // these are node dim
  uint32_t dimX = 8;
  uint32_t dimY = 10;
  uint32_t dimZ = 12;
  glm::vec3 bound_low{-3., -3., -3.};
  glm::vec3 bound_high{3., 3., 3.};

  polyscope::VolumeGrid* psGrid = polyscope::registerVolumeGrid("test grid", {dimX, dimY, dimZ}, bound_low, bound_high);

  // callable, for callable variants below
  auto torusSDF = [](glm::vec3 p) {
    float scale = 0.5;
    p /= scale;
    p += glm::vec3{1., 0., 1.};
    glm::vec2 t{1., 0.3};
    glm::vec2 pxz{p.x, p.z};
    glm::vec2 q = glm::vec2(glm::length(pxz) - t.x, p.y);
    return (glm::length(q) - t.y) * scale;
  };

  // node scalar from callable
  // internally this bootstraps off the batch version, so we're kinda testing it too
  polyscope::VolumeGridNodeScalarQuantity* q = psGrid->addNodeScalarQuantityFromCallable("node scalar2", torusSDF);
  q->setEnabled(true);

  q->setGridcubeVizEnabled(false);
  polyscope::show(3);
  
  q->setIsosurfaceVizEnabled(true); // extracts the isosurface
  polyscope::show(3);
  
  polyscope::SlicePlane* p = polyscope::addSceneSlicePlane();
  polyscope::show(3);

  q->setSlicePlanesAffectIsosurface(true); 
  polyscope::show(3);

  q->registerIsosurfaceAsMesh();

  // this setting should mean we get no isosurface, make sure nothing crashes
  q->setIsosurfaceLevel(10000.);
  polyscope::show(3);
  q->registerIsosurfaceAsMesh();
  polyscope::show(3);

  polyscope::removeLastSceneSlicePlane();
  polyscope::removeAllStructures();
}
