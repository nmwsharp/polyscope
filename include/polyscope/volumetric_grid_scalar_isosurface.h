#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/color_maps.h"
#include "polyscope/histogram.h"
#include "polyscope/volumetric_grid.h"

namespace polyscope {

class VolumetricGridScalarIsosurface : public VolumetricGridQuantity {
public:
  VolumetricGridScalarIsosurface(std::string name, VolumetricGrid& grid_, const std::vector<double> &values_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;
  virtual void geometryChanged() override;

protected:
  std::vector<double> values;
  void createProgram();
  void setProgramUniforms(gl::GLProgram& program);
  std::unique_ptr<gl::GLProgram> meshProgram;
};

} // namespace polyscope
