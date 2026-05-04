// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/affine_remapper.h"
#include "polyscope/render/color_maps.h"
#include "polyscope/scalar_quantity.h"
#include "polyscope/simple_triangle_mesh_quantity.h"
#include "polyscope/standardize_data_array.h"

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

  // Throws if the quantity buffer size doesn't match the parent mesh's vertex/face count.
  void checkQuantitySizeMatchesParentStructure();

  // Update the scalar values, allowing the count to change (e.g. after updateMesh()).
  // For same-size updates, prefer ScalarQuantity::updateData() which validates the size.
  template <class V>
  void updateData(const V& newValues) {
    std::vector<float> newData = standardizeArray<float, V>(newValues);
    values.resize(newData.size());
    values.data.assign(newData.begin(), newData.end());
    values.markHostBufferUpdated();
  }

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
  virtual void buildVertexInfoGUI(size_t vInd) override;
};


// ========================================================
// ==========           Face Scalar              ==========
// ========================================================

class SimpleTriangleMeshFaceScalarQuantity : public SimpleTriangleMeshScalarQuantity {
public:
  SimpleTriangleMeshFaceScalarQuantity(std::string name, const std::vector<float>& values, SimpleTriangleMesh& mesh,
                                       DataType dataType = DataType::STANDARD);

  virtual void createProgram() override;
  virtual void buildFaceInfoGUI(size_t fInd) override;
};


} // namespace polyscope
