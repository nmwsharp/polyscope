// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/custom_shader_quantity.h"
#include "polyscope/surface_mesh.h"

namespace polyscope {

// forward declarations
class SurfaceMeshQuantity;
class SurfaceMesh;

class SurfaceCustomShaderQuantity : public SurfaceMeshQuantity, public CustomShaderQuantity {
public:
  SurfaceCustomShaderQuantity(std::string name, SurfaceMesh& mesh_, std::string programText);

  virtual void draw() override;
  virtual std::string niceName() override;
  virtual void refresh() override;

  void addAttribute(std::string quantityName);

protected:
  // UI internals

  // Helpers
  virtual void createProgram() = 0;

  virtual void resolveAttribute(CustomShaderAttributeEntry& entry) override;
};

} // namespace polyscope
