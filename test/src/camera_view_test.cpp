// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/camera_parameters.h"
#include "polyscope/color_image_quantity.h"
#include "polyscope_test.h"

// ============================================================
// =============== Camera View Test
// ============================================================

// Add floating images

TEST_F(PolyscopeTest, AddCameraView) {

  polyscope::CameraView* cam1 = polyscope::registerCameraView(
      "cam1", polyscope::CameraParameters(polyscope::CameraIntrinsics::fromFoVDegVerticalAndAspect(60, 2.),
                                          polyscope::CameraExtrinsics::fromVectors(
                                              glm::vec3{2., 2., 2.}, glm::vec3{-1., -1., -1.}, glm::vec3{0., 1., 0.})));

  EXPECT_TRUE(polyscope::hasCameraView("cam1"));
  EXPECT_TRUE(polyscope::getCameraView("cam1") != nullptr);

  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CameraViewOptions) {

  polyscope::CameraView* cam1 = polyscope::registerCameraView(
      "cam1", polyscope::CameraParameters(polyscope::CameraIntrinsics::fromFoVDegVerticalAndAspect(60, 2.),
                                          polyscope::CameraExtrinsics::fromVectors(
                                              glm::vec3{2., 2., 2.}, glm::vec3{-1., -1., -1.}, glm::vec3{0., 1., 0.})));

  cam1->setWidgetFocalLength(0.75, false);
  EXPECT_EQ(cam1->getWidgetFocalLength(), 0.75);

  cam1->setWidgetThickness(0.25);
  EXPECT_EQ(cam1->getWidgetThickness(), 0.25);

  glm::vec3 c = glm::vec3{0.25, 0.25, 0.25};
  cam1->setWidgetColor(c);
  EXPECT_EQ(cam1->getWidgetColor(), c);


  polyscope::show(3);
  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, CameraViewUpdate) {

  polyscope::CameraView* cam1 = polyscope::registerCameraView(
      "cam1", polyscope::CameraParameters(polyscope::CameraIntrinsics::fromFoVDegVerticalAndAspect(60, 2.),
                                          polyscope::CameraExtrinsics::fromVectors(
                                              glm::vec3{2., 2., 2.}, glm::vec3{-1., -1., -1.}, glm::vec3{0., 1., 0.})));

  polyscope::show(3);

  cam1->updateCameraParameters(
      polyscope::CameraParameters(polyscope::CameraIntrinsics::fromFoVDegVerticalAndAspect(65, 3.),
                                  polyscope::CameraExtrinsics::fromVectors(
                                      glm::vec3{3., 2., 2.}, glm::vec3{-1., -1., -2.}, glm::vec3{1., 1., 0.})));

  polyscope::show(3);

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, AddCameraViewColorImage) {

  polyscope::CameraView* cam1 = polyscope::registerCameraView(
      "cam1", polyscope::CameraParameters(polyscope::CameraIntrinsics::fromFoVDegVerticalAndAspect(60, 2.),
                                          polyscope::CameraExtrinsics::fromVectors(
                                              glm::vec3{2., 2., 2.}, glm::vec3{-1., -1., -1.}, glm::vec3{0., 1., 0.})));

  int width = 300;
  int height = 400;
  std::vector<std::array<float, 3>> imageColor(width * height);
  polyscope::ColorImageQuantity* im =
      cam1->addColorImageQuantity("test color image", width, height, imageColor, polyscope::ImageOrigin::UpperLeft);
  im->setEnabled(true);

  polyscope::show(3);

  im->setShowInCameraBillboard(true);
  polyscope::show(3);

  // make sure it doesn't blow up with transparancy
  polyscope::options::transparencyMode = polyscope::TransparencyMode::Simple;
  polyscope::show(3);

  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::show(3);

  polyscope::options::transparencyMode = polyscope::TransparencyMode::None;

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, AddCameraViewColorAlphaImage) {

  polyscope::CameraView* cam1 = polyscope::registerCameraView(
      "cam1", polyscope::CameraParameters(polyscope::CameraIntrinsics::fromFoVDegVerticalAndAspect(60, 2.),
                                          polyscope::CameraExtrinsics::fromVectors(
                                              glm::vec3{2., 2., 2.}, glm::vec3{-1., -1., -1.}, glm::vec3{0., 1., 0.})));

  int width = 300;
  int height = 400;
  std::vector<std::array<float, 4>> imageColor(width * height);
  polyscope::ColorImageQuantity* im = cam1->addColorAlphaImageQuantity("test color alpha image", width, height,
                                                                       imageColor, polyscope::ImageOrigin::UpperLeft);
  im->setEnabled(true);

  polyscope::show(3);

  im->setShowInCameraBillboard(true);
  polyscope::show(3);

  // make sure it doesn't blow up with transparancy
  polyscope::options::transparencyMode = polyscope::TransparencyMode::Simple;
  polyscope::show(3);

  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::show(3);

  polyscope::options::transparencyMode = polyscope::TransparencyMode::None;

  polyscope::removeAllStructures();
}

TEST_F(PolyscopeTest, AddCameraViewScalarImage) {

  polyscope::CameraView* cam1 = polyscope::registerCameraView(
      "cam1", polyscope::CameraParameters(polyscope::CameraIntrinsics::fromFoVDegVerticalAndAspect(60, 2.),
                                          polyscope::CameraExtrinsics::fromVectors(
                                              glm::vec3{2., 2., 2.}, glm::vec3{-1., -1., -1.}, glm::vec3{0., 1., 0.})));

  int width = 300;
  int height = 400;
  std::vector<float> imageScalar(width * height);
  polyscope::ScalarImageQuantity* im =
      cam1->addScalarImageQuantity("test scalar image", width, height, imageScalar, polyscope::ImageOrigin::UpperLeft);
  im->setEnabled(true);

  polyscope::show(3);

  im->setShowInCameraBillboard(true);
  polyscope::show(3);

  // make sure it doesn't blow up with transparancy
  polyscope::options::transparencyMode = polyscope::TransparencyMode::Simple;
  polyscope::show(3);

  polyscope::options::transparencyMode = polyscope::TransparencyMode::Pretty;
  polyscope::show(3);

  polyscope::options::transparencyMode = polyscope::TransparencyMode::None;

  polyscope::removeAllStructures();
}
