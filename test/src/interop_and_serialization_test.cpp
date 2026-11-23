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

TEST_F(PolyscopeTest, ManagedBufferCapacity) {
  polyscope::render::ManagedBuffer<float> mb(nullptr, "cap_test", std::vector<float>{1.f, 2.f, 3.f, 4.f});
  EXPECT_EQ(mb.size(), 4u);
  EXPECT_EQ(mb.capacity(), 4u);
  EXPECT_EQ(mb.getDataCopy(), (std::vector<float>{1.f, 2.f, 3.f, 4.f}));

  // Explicit capacity growth preserves size and data; shrinking below size throws
  mb.setCapacity(10);
  EXPECT_EQ(mb.size(), 4u);
  EXPECT_EQ(mb.capacity(), 10u);
  EXPECT_EQ(mb.getDataCopy(), (std::vector<float>{1.f, 2.f, 3.f, 4.f}));
  EXPECT_THROW(mb.setCapacity(2), std::exception);

  // Within-capacity resize does not reallocate; out-of-bounds respects size(), not capacity()
  EXPECT_FALSE(mb.resize(6));
  EXPECT_EQ(mb.size(), 6u);
  EXPECT_EQ(mb.capacity(), 10u);
  mb.setDataHost({10.f, 20.f, 30.f, 40.f, 50.f, 60.f});
  EXPECT_FALSE(mb.resize(3));
  EXPECT_EQ(mb.getDataCopy(), (std::vector<float>{10.f, 20.f, 30.f}));
  EXPECT_THROW(mb.getValue(3), std::exception);
  EXPECT_THROW(mb.setDataHost(std::vector<float>(11, 0.f)), std::exception);

  // GPU buffer stays allocated at capacity across within-capacity resizes and reallocates in-place on growth
  auto gpuBuf = mb.getRenderAttributeBuffer();
  EXPECT_EQ(gpuBuf->getBufferSize(), mb.capacity());

  EXPECT_FALSE(mb.resize(5));
  mb.syncToDeviceIfNeeded();
  EXPECT_EQ(mb.getRenderAttributeBuffer(), gpuBuf);
  EXPECT_EQ(gpuBuf->getBufferSize(), 10u);

  // Growth beyond capacity triggers amortized doubling and in-place GPU reallocation
  EXPECT_TRUE(mb.resize(12));
  EXPECT_EQ(mb.size(), 12u);
  EXPECT_GE(mb.capacity(), 20u);
  EXPECT_EQ(mb.getValue(0), 10.f);
  EXPECT_EQ(mb.getValue(2), 30.f);
  mb.syncToDeviceIfNeeded();
  EXPECT_EQ(mb.getRenderAttributeBuffer(), gpuBuf);
  EXPECT_EQ(gpuBuf->getBufferSize(), mb.capacity());

  // Shrinking capacity also resizes the underlying GPU buffer to match
  mb.setCapacity(12);
  EXPECT_EQ(mb.capacity(), 12u);
  EXPECT_EQ(gpuBuf->getBufferSize(), 12u);
}

TEST_F(PolyscopeTest, ManagedBufferDeviceSync) {
  polyscope::render::ManagedBuffer<float> mb(nullptr, "sync_test", std::vector<float>{1.f, 2.f, 3.f, 4.f});
  mb.setCapacity(8);
  auto gpuBuf = mb.getRenderAttributeBuffer();

  // Simulate external GPU write (e.g., CUDA-OpenGL interop)
  std::vector<float> gpuVals = {10.f, 20.f, 30.f, 40.f, 0.f, 0.f, 0.f, 0.f};
  gpuBuf->setData(gpuVals);
  mb.markRenderAttributeBufferUpdated();

  // Single-element and full readback
  float v2 = mb.getValue(2);
  std::vector<float> hostCopy = mb.getDataCopy();
  EXPECT_EQ(hostCopy.size(), 4u);
  if (testBackend != "openGL_mock") {
    EXPECT_FLOAT_EQ(v2, 30.f);
    EXPECT_EQ(hostCopy, (std::vector<float>{10.f, 20.f, 30.f, 40.f}));
  }

  // Within-capacity resize while device-canonical should not clobber GPU data
  gpuVals[1] = 77.f;
  gpuBuf->setData(gpuVals);
  mb.markRenderAttributeBufferUpdated();
  EXPECT_FALSE(mb.resize(2));
  EXPECT_EQ(mb.size(), 2u);
  if (testBackend != "openGL_mock") {
    EXPECT_FLOAT_EQ(mb.getValue(1), 77.f);
  }
  EXPECT_FALSE(mb.resize(4));

  // Growing while device-canonical should read back valid elements before reallocating
  gpuVals[0] = 99.f;
  gpuBuf->setData(gpuVals);
  mb.markRenderAttributeBufferUpdated();
  EXPECT_TRUE(mb.resize(12));
  EXPECT_EQ(mb.size(), 12u);
  EXPECT_GE(mb.capacity(), 16u);
  if (testBackend != "openGL_mock") {
    EXPECT_FLOAT_EQ(mb.getValue(0), 99.f);
    EXPECT_FLOAT_EQ(mb.getValue(3), 40.f);
  }
}

TEST_F(PolyscopeTest, ManagedBufferLazyCompute) {
  int computeCount = 0;
  polyscope::render::ManagedBuffer<float> mb(nullptr, "lazy_test", [&]() {
    computeCount++;
    mb.resize(3);
    mb.setDataHost({static_cast<float>(computeCount), 2.f, 3.f});
  });

  // Should not compute before first access
  mb.recomputeIfPopulated();
  EXPECT_EQ(computeCount, 0);

  // First access triggers computation once
  EXPECT_FLOAT_EQ(mb.getValue(0), 1.f);
  EXPECT_FLOAT_EQ(mb.getValue(2), 3.f);
  EXPECT_EQ(computeCount, 1);

  // Recompute now runs since buffer is populated
  mb.recomputeIfPopulated();
  EXPECT_EQ(computeCount, 2);
  EXPECT_FLOAT_EQ(mb.getValue(0), 2.f);
}

TEST_F(PolyscopeTest, ManagedBufferIndexedViews) {
  polyscope::render::ManagedBuffer<float> src(nullptr, "idx_src", std::vector<float>{10.f, 20.f, 30.f, 40.f});
  polyscope::render::ManagedBuffer<uint32_t> indices(nullptr, "idx_ind", std::vector<uint32_t>{3, 1, 0, 1});

  EXPECT_EQ(src.getIndexedDataCopy(indices), (std::vector<float>{40.f, 20.f, 10.f, 20.f}));

  // Indexed render buffer is cached and updated lazily on sync
  auto viewBuf = src.getIndexedRenderAttributeBuffer(indices);
  EXPECT_EQ(src.getIndexedRenderAttributeBuffer(indices), viewBuf);

  src.setDataHost({1.f, 2.f, 3.f, 4.f});
  src.syncToDeviceIfNeeded();
  EXPECT_EQ(src.getIndexedDataCopy(indices), (std::vector<float>{4.f, 2.f, 1.f, 2.f}));

  // Shrinking indices within capacity should only gather up to indices.size()
  EXPECT_FALSE(indices.resize(2));
  EXPECT_EQ(src.getIndexedDataCopy(indices), (std::vector<float>{4.f, 2.f}));
}

TEST_F(PolyscopeTest, ManagedBufferTextures) {
  // 1D texture: capacity growth after GPU buffer allocation
  polyscope::render::ManagedBuffer<float> tex1d(nullptr, "tex1d_test");
  tex1d.setAsType(polyscope::DeviceBufferType::Texture1d);
  tex1d.setCapacity(4);
  tex1d.setDataHost({1.f, 2.f, 3.f, 4.f});
  auto gpuTex1d = tex1d.getRenderTextureBuffer();
  EXPECT_TRUE(tex1d.resize(8));
  tex1d.setDataHost(std::vector<float>(8, 5.f));
  EXPECT_NO_THROW(tex1d.syncToDeviceIfNeeded());
  EXPECT_EQ(tex1d.getRenderTextureBuffer(), gpuTex1d);

  // 2D texture: empty construction -> setTextureSize -> populate -> resize
  polyscope::render::ManagedBuffer<float> tex2d(nullptr, "tex2d_test");
  tex2d.setAsType(polyscope::DeviceBufferType::Texture2d);
  tex2d.setTextureSize(2, 3);
  tex2d.setDataHost(std::vector<float>(6, 1.f));
  auto gpuTex2d = tex2d.getRenderTextureBuffer();
  tex2d.setTextureSize(4, 2);
  tex2d.setDataHost(std::vector<float>(8, 2.f));
  EXPECT_NO_THROW(tex2d.syncToDeviceIfNeeded());
  EXPECT_EQ(tex2d.getRenderTextureBuffer(), gpuTex2d);
}
