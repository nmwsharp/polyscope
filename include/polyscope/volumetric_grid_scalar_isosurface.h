#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/color_maps.h"
#include "polyscope/histogram.h"
#include "polyscope/volumetric_grid.h"

namespace polyscope {

class VolumetricGridScalarIsosurface : public VolumetricGridQuantity {
public:
  VolumetricGridScalarIsosurface(std::string name, VolumetricGrid& grid_, const std::vector<double> &values_, double levelSet_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;
  virtual void geometryChanged() override;

  void recomputeNormals();
  void meshIsosurface();

protected:
  std::vector<double> values;
  double levelSet;
  double newLevelSet;
  double increment;
  void prepare();
  void prepareTriangleIndices();
  void setProgramUniforms(gl::GLProgram& program);
  void fillGeometryBuffersSmooth(gl::GLProgram& p);
  std::unique_ptr<gl::GLProgram> meshProgram;
  std::vector<glm::vec3> nodes;
  std::vector<glm::vec3> normals;
  std::vector<std::array<size_t, 3>> triangles;
};

} // namespace polyscope
