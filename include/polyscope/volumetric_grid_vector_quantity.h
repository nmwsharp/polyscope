#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/gl/color_maps.h"
#include "polyscope/histogram.h"
#include "polyscope/volumetric_grid.h"

namespace polyscope {
class VolumetricGridVectorQuantity : public VolumetricGridQuantity {
public:
  VolumetricGridVectorQuantity(std::string name, VolumetricGrid& grid_, const std::vector<glm::vec3>& values_,
                               VectorType vectorType_);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual std::string niceName() override;
  virtual void geometryChanged() override;

protected:
  VectorType vectorType;
  std::vector<glm::vec3> vectorValues;
  std::vector<glm::vec3> positions;
  std::unique_ptr<gl::GLProgram> vectorsProgram;
  AffineRemapper<glm::vec3> mapper;
  float vectorRadius;
  float vectorLengthMult;

  void fillPositions();
  void createProgram();
};
} // namespace polyscope