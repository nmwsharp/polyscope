#pragma once

#include "polyscope/surface_mesh.h"

namespace polyscope {

class SurfaceScalarQuantity :  public SurfaceQuantityThatDrawsFaces {
 public:
  SurfaceScalarQuantity(std::string name, SurfaceMesh* mesh_,
                        DataType dataType);

  // === Members
  const DataType dataType;

  // The map that takes values to [0,1] for drawing
  double mapVal(double x);
  double minVal, maxVal;

  // UI internals
  const std::vector<const gl::Colormap*> colormaps = {&gl::CM_VIRIDIS, &gl::CM_COOLWARM, &gl::CM_BLUES};
  const char* cm_names[3] = {"viridis", "coolwarm", "blues"};
  int iColorMap = 0;
};

class SurfaceScalarVertexQuantity : public SurfaceScalarQuantity {
 public:
  SurfaceScalarVertexQuantity(std::string name, VertexData<double>& values_,
                              SurfaceMesh* mesh_,
                              DataType dataType_ = DataType::STANDARD);
  //   ~SurfaceScalarVertexQuantity();

  virtual void draw() override;
  virtual void drawUI() override;
  virtual gl::GLProgram* createProgram() override;

  void fillColorBuffers(gl::GLProgram* p);

  // === Members
  VertexData<double> values;

};

}  // namespace polyscope