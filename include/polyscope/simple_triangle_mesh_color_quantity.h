// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/color_quantity.h"
#include "polyscope/simple_triangle_mesh_quantity.h"
#include "polyscope/standardize_data_array.h"

#include <vector>

namespace polyscope {

class SimpleTriangleMeshColorQuantity : public SimpleTriangleMeshQuantity,
                                        public ColorQuantity<SimpleTriangleMeshColorQuantity> {
public:
  SimpleTriangleMeshColorQuantity(std::string name, const std::vector<glm::vec3>& colors, std::string definedOn,
                                  SimpleTriangleMesh& mesh);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void buildColorOptionsUI() override;
  virtual void refresh() override;
  virtual std::string niceName() override;

  // Throws if the quantity buffer size doesn't match the parent mesh's vertex/face count.
  void checkQuantitySizeMatchesParentStructure();

  // Update the color values, allowing the count to change (e.g. after updateMesh()).
  // For same-size updates, prefer ColorQuantity::updateData() which validates the size.
  template <class V>
  void updateData(const V& newColors) {
    std::vector<glm::vec3> newData = standardizeVectorArray<glm::vec3, 3>(newColors);
    colors.resize(newData.size());
    colors.data.assign(newData.begin(), newData.end());
    colors.markHostBufferUpdated();
  }

  const std::string definedOn;

protected:
  std::shared_ptr<render::ShaderProgram> program;

  virtual void createProgram() = 0;
};


// ========================================================
// ==========          Vertex Color              ==========
// ========================================================

class SimpleTriangleMeshVertexColorQuantity : public SimpleTriangleMeshColorQuantity {
public:
  SimpleTriangleMeshVertexColorQuantity(std::string name, const std::vector<glm::vec3>& colors,
                                        SimpleTriangleMesh& mesh);

  virtual void createProgram() override;
  virtual void buildVertexInfoGUI(size_t vInd) override;
};


// ========================================================
// ==========           Face Color               ==========
// ========================================================

class SimpleTriangleMeshFaceColorQuantity : public SimpleTriangleMeshColorQuantity {
public:
  SimpleTriangleMeshFaceColorQuantity(std::string name, const std::vector<glm::vec3>& colors,
                                      SimpleTriangleMesh& mesh);

  virtual void createProgram() override;
  virtual void buildFaceInfoGUI(size_t fInd) override;
};


} // namespace polyscope
