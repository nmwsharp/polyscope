// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/scalar_quantity.h"
#include "polyscope/simple_triangle_mesh_quantity.h"

#include <vector>

namespace polyscope {

class SimpleTriangleMeshScalarQuantity : public SimpleTriangleMeshQuantity,
                                         public ScalarQuantity<SimpleTriangleMeshScalarQuantity> {
public:
  SimpleTriangleMeshScalarQuantity(std::string name, const std::vector<float>& values, std::string definedOn,
                                   SimpleTriangleMesh& mesh, DataType dataType);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;
  virtual std::string niceName() override;


protected:
  std::string definedOn;
  std::shared_ptr<render::ShaderProgram> program;

  void createProgram();
};

} // namespace polyscope
