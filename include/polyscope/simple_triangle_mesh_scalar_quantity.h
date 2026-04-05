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

  const std::string definedOn;

protected:
  std::shared_ptr<render::ShaderProgram> program;

  virtual void createProgram() = 0;
};


// ========================================================
// ==========          Vertex Scalar             ==========
// ========================================================

class SimpleTriangleMeshVertexScalarQuantity : public SimpleTriangleMeshScalarQuantity {
public:
  SimpleTriangleMeshVertexScalarQuantity(std::string name, const std::vector<float>& values, SimpleTriangleMesh& mesh,
                                         DataType dataType = DataType::STANDARD);

  virtual void createProgram() override;
};


// ========================================================
// ==========           Face Scalar              ==========
// ========================================================

class SimpleTriangleMeshFaceScalarQuantity : public SimpleTriangleMeshScalarQuantity {
public:
  SimpleTriangleMeshFaceScalarQuantity(std::string name, const std::vector<float>& values, SimpleTriangleMesh& mesh,
                                       DataType dataType = DataType::STANDARD);

  virtual void createProgram() override;
};


} // namespace polyscope
