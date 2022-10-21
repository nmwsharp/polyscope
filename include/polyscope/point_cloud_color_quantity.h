// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/histogram.h"
#include "polyscope/point_cloud.h"
#include "polyscope/point_cloud_quantity.h"

#include <vector>

namespace polyscope {

class PointCloudColorQuantity : public PointCloudQuantity {
public:
  PointCloudColorQuantity(std::string name, const std::vector<glm::vec3>& values, PointCloud& pointCloud_);

  virtual void draw() override;

  virtual void buildPickUI(size_t ind) override;
  virtual void refresh() override;

  virtual std::string niceName() override;
  
  template <class V>
  void updateData(const V& newColors);

  void ensureRenderBuffersFilled(bool forceRefill=false);

  std::shared_ptr<render::AttributeBuffer> getColorRenderBuffer();

  // === Members
  std::vector<glm::vec3> values;
  
  // === ~DANGER~ experimental/unsupported functions

  uint32_t getColorBufferID();
  void bufferDataExternallyUpdated();

protected:
  void createPointProgram();
  void dataUpdated();

  std::shared_ptr<render::AttributeBuffer> colorBuffer;
  std::shared_ptr<render::ShaderProgram> pointProgram;
};


} // namespace polyscope

#include "polyscope/point_cloud_color_quantity.ipp"
