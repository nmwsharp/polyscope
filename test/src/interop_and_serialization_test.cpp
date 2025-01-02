// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope_test.h"


// ============================================================
// =============== Managed Buffer Access
// ============================================================

TEST_F(PolyscopeTest, ManagedBufferAccess) {

  // register point cloud
  auto psPoints = registerPointCloud("test_cloud1");
  std::vector<double> vScalar(psPoints->nPoints(), 7.);
  auto q2 = psPoints->addScalarQuantity("vScalar", vScalar);
  q2->setEnabled(true);


  // make sure we can get its buffers
  polyscope::render::ManagedBuffer<glm::vec3>& bufferPos = psPoints->getManagedBuffer<glm::vec3>("points");
  polyscope::render::ManagedBuffer<float>& bufferScalar = q2->getManagedBuffer<float>("values");


  size_t dimX = 300;
  size_t dimY = 200;
  // register an image quantity
  std::vector<std::array<float, 3>> valsRGB(dimX * dimY, std::array<float, 3>{0.44, 0.55, 0.66});
  polyscope::ColorImageQuantity* im =
      polyscope::addColorImageQuantity("im color", dimX, dimY, valsRGB, polyscope::ImageOrigin::UpperLeft);

  // make sure we can get its buffers
  polyscope::render::ManagedBuffer<glm::vec4>& bufferColor = im->getManagedBuffer<glm::vec4>("colors");

  polyscope::removeAllStructures();
}
