// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/parameterization_quantity.h"
#include "polyscope/point_cloud.h"
#include "polyscope/point_cloud_quantity.h"

#include <vector>

namespace polyscope {

class PointCloudParameterizationQuantity : public PointCloudQuantity,
                                           public ParameterizationQuantity<PointCloudParameterizationQuantity> {
public:
  PointCloudParameterizationQuantity(std::string name, PointCloud& cloud_, const std::vector<glm::vec2>& coords_,
                                     ParamCoordsType type_, ParamVizStyle style_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void buildPickUI(size_t ind) override;
  virtual void refresh() override;
  virtual std::string niceName() override;


protected:
  std::shared_ptr<render::ShaderProgram> program;

  // Helpers
  void createProgram();
  void fillCoordBuffers(render::ShaderProgram& p);
};


} // namespace polyscope
