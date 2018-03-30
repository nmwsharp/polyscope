#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/colormap_sets.h"
#include "polyscope/histogram.h"
#include "polyscope/point_cloud.h"

#include <vector>

namespace polyscope {

class PointCloudColorQuantity : public PointCloudQuantityThatDrawsPoints {
public:
  PointCloudColorQuantity(std::string name, const std::vector<Vector3>& values, PointCloud* pointCloud_);

  virtual void drawUI() override;
  virtual bool wantsBillboardUniforms() override;

  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);
  void buildInfoGUI(size_t ind) override;

  // === Members
  std::vector<Vector3> values;
};


} // namespace polyscope
