#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/color_maps.h"
#include "polyscope/histogram.h"
#include "polyscope/volumetric_grid.h"

namespace polyscope {

class VolumetricGridScalarQuantity : public VolumetricGridQuantity {
public:
  VolumetricGridScalarQuantity(std::string name, VolumetricGrid& grid_, const std::vector<double>& values_, DataType dataType_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;
  virtual void geometryChanged() override;

protected:
  void createProgram();
  void fillGeometryBuffersSmooth(gl::GLProgram& p);
  void fillPositions();
  void setPointCloudUniforms();
  void resetMapRange();

  const DataType dataType;
  std::vector<double> values;
  std::vector<glm::vec3> positions;
  PersistentValue<gl::ColorMapID> cMap;
  std::unique_ptr<gl::GLProgram> pointsProgram;
  float pointRadius;
  std::pair<float, float> vizRange;
  std::pair<double, double> dataRange;
};

} // namespace polyscope
