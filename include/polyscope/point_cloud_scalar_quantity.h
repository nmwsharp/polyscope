// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/point_cloud.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/scalar_quantity.h"

#include <vector>

namespace polyscope {

class PointCloudScalarQuantity : public PointCloudQuantity, public ScalarQuantity<PointCloudScalarQuantity> {

public:
  PointCloudScalarQuantity(std::string name, const std::vector<double>& values, PointCloud& pointCloud_,
                           DataType dataType);

  virtual void draw() override;
  virtual void buildCustomUI() override;

  virtual void buildPickUI(size_t ind) override;
  virtual void refresh() override;
  
  template <class V>
  void updateData(const V& newScalars);

  void ensureRenderBuffersFilled(bool forceRefill=false);

  std::shared_ptr<render::AttributeBuffer> getScalarRenderBuffer();

  virtual std::string niceName() override;

  // === ~DANGER~ experimental/unsupported functions

  uint32_t getScalarBufferID();
  void bufferDataExternallyUpdated();

protected:
  void createProgram();
  void dataUpdated();

  std::shared_ptr<render::AttributeBuffer> scalarRenderBuffer;
  std::shared_ptr<render::ShaderProgram> pointProgram;
};


} // namespace polyscope

#include "polyscope/point_cloud_scalar_quantity.ipp"
