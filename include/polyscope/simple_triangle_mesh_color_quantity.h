// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/color_quantity.h"
#include "polyscope/simple_triangle_mesh_quantity.h"

#include <vector>

namespace polyscope {

class SimpleTriangleMeshColorQuantity : public SimpleTriangleMeshQuantity,
                                        public ColorQuantity<SimpleTriangleMeshColorQuantity> {
public:
  SimpleTriangleMeshColorQuantity(std::string name, const std::vector<glm::vec3>& colors, std::string definedOn,
                                  SimpleTriangleMesh& mesh);

  virtual void draw() override;
  virtual void buildCustomUI() override;
  virtual void refresh() override;
  virtual std::string niceName() override;

  std::string definedOn;

protected:
  void createProgram();
  std::shared_ptr<render::ShaderProgram> program;
};

} // namespace polyscope
